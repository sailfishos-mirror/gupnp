/* 
 * Copyright (C) 2006, 2007 OpenedHand Ltd.
 *
 * Author: Jorn Baayen <jorn@openedhand.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:gupnp-service-info
 * @short_description: Interface for querying service information.
 *
 * The #GUPnPDeviceInfo interface provides methods for querying service
 * information.
 */

#include <libsoup/soup.h>
#include <string.h>

#include "gupnp-service-info.h"
#include "gupnp-context-private.h"
#include "xml-util.h"

G_DEFINE_ABSTRACT_TYPE (GUPnPServiceInfo,
                        gupnp_service_info,
                        G_TYPE_OBJECT);

struct _GUPnPServiceInfoPrivate {
        GUPnPContext *context;

        char *location;
        char *udn;
        char *url_base;

        xmlNode *element;
};

enum {
        PROP_0,
        PROP_CONTEXT,
        PROP_LOCATION,
        PROP_UDN,
        PROP_URL_BASE,
        PROP_ELEMENT
};

static void
gupnp_service_info_init (GUPnPServiceInfo *info)
{
        info->priv = G_TYPE_INSTANCE_GET_PRIVATE (info,
                                                  GUPNP_TYPE_SERVICE_INFO,
                                                  GUPnPServiceInfoPrivate);
}

static void
gupnp_service_info_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
        GUPnPServiceInfo *info;

        info = GUPNP_SERVICE_INFO (object);

        switch (property_id) {
        case PROP_CONTEXT:
                info->priv->context =
                        g_object_ref (g_value_get_object (value));
                break;
        case PROP_LOCATION:
                g_free (info->priv->location);
                info->priv->location =
                        g_value_dup_string (value);
                break;
        case PROP_UDN:
                g_free (info->priv->udn);
                info->priv->udn =
                        g_value_dup_string (value);
                break;
        case PROP_URL_BASE:
                g_free (info->priv->url_base);
                info->priv->url_base =
                        g_value_dup_string (value);
                break;
        case PROP_ELEMENT:
                info->priv->element =
                        g_value_get_pointer (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_service_info_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
        GUPnPServiceInfo *info;

        info = GUPNP_SERVICE_INFO (object);

        switch (property_id) {
        case PROP_CONTEXT:
                g_value_set_object (value,
                                    info->priv->context);
                break;
        case PROP_LOCATION:
                g_value_set_string (value,
                                    info->priv->location);
                break;
        case PROP_UDN:
                g_value_set_string (value,
                                    info->priv->udn);
                break;
        case PROP_URL_BASE:
                g_value_set_string (value,
                                    info->priv->url_base);
                break;
        case PROP_ELEMENT:
                g_value_set_pointer (value,
                                     info->priv->element);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_service_info_dispose (GObject *object)
{
        GUPnPServiceInfo *info;

        info = GUPNP_SERVICE_INFO (object);

        if (info->priv->context) {
                g_object_unref (info->priv->context);
                info->priv->context = NULL;
        }
}

static void
gupnp_service_info_finalize (GObject *object)
{
        GUPnPServiceInfo *info;

        info = GUPNP_SERVICE_INFO (object);

        g_free (info->priv->location);
        g_free (info->priv->udn);
        g_free (info->priv->url_base);
}

static void
gupnp_service_info_class_init (GUPnPServiceInfoClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = gupnp_service_info_set_property;
        object_class->get_property = gupnp_service_info_get_property;
        object_class->dispose      = gupnp_service_info_dispose;
        object_class->finalize     = gupnp_service_info_finalize;

        g_type_class_add_private (klass, sizeof (GUPnPServiceInfoPrivate));

        /**
         * GUPnPServiceInfo:context
         *
         * The #GUPnPContext to use.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_CONTEXT,
                 g_param_spec_object ("context",
                                      "Context",
                                      "The GUPnPContext.",
                                      GUPNP_TYPE_CONTEXT,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPServiceInfo:location
         *
         * The location of the device description file.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_LOCATION,
                 g_param_spec_string ("location",
                                      "Location",
                                      "The location of the device description "
                                      "file",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPServiceInfo:udn
         *
         * The UDN of the containing device.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_UDN,
                 g_param_spec_string ("udn",
                                      "UDN",
                                      "The UDN of the containing device",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPServiceInfo:url-base
         *
         * The URL base.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_URL_BASE,
                 g_param_spec_string ("url-base",
                                      "URL base",
                                      "The URL base",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPServiceInfo:element
         *
         * Private property.
         *
         * Stability: Private
         **/
        g_object_class_install_property
                (object_class,
                 PROP_ELEMENT,
                 g_param_spec_pointer ("element",
                                       "Element",
                                       "The XML element related to this device",
                                       G_PARAM_READWRITE |
                                       G_PARAM_CONSTRUCT_ONLY |
                                       G_PARAM_STATIC_NAME |
                                       G_PARAM_STATIC_NICK |
                                       G_PARAM_STATIC_BLURB));
}

/**
 * gupnp_service_info_get_context
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The #GUPnPContext associated with @info.
 **/
GUPnPContext *
gupnp_service_info_get_context (GUPnPServiceInfo *info)
{
        g_return_val_if_fail (GUPNP_IS_SERVICE_INFO (info), NULL);

        return info->priv->context;
}

/**
 * gupnp_service_info_get_location
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The location of the device description file.
 **/
const char *
gupnp_service_info_get_location (GUPnPServiceInfo *info)
{
        g_return_val_if_fail (GUPNP_IS_SERVICE_INFO (info), NULL);

        return info->priv->location;
}

/**
 * gupnp_service_info_get_url_base
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The URL base.
 **/
const char *
gupnp_service_info_get_url_base (GUPnPServiceInfo *info)
{
        g_return_val_if_fail (GUPNP_IS_SERVICE_INFO (info), NULL);

        return info->priv->url_base;
}

/**
 * gupnp_service_info_get_udn
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The UDN of the containing device.
 **/
const char *
gupnp_service_info_get_udn (GUPnPServiceInfo *info)
{
        g_return_val_if_fail (GUPNP_IS_SERVICE_INFO (info), NULL);

        return info->priv->udn;
}

static char *
get_property (GUPnPServiceInfo *info,
              const char       *element_name)
{
        xmlNode *element;

        g_return_val_if_fail (GUPNP_IS_SERVICE_INFO (info), NULL);

        element = xml_util_get_element (info->priv->element,
                                        element_name,
                                        NULL);

        if (element) {
                xmlChar *value;
                char *ret;
                
                /* Make glib memmanaged */
                value = xmlNodeGetContent (element);
                ret = g_strdup ((char *) value);
                xmlFree (value);

                return ret;
        } else
                return NULL;
}

static char *
get_url_property (GUPnPServiceInfo *info,
                  const char       *element_name)
{
        char *prop;

        prop = get_property (info, element_name);

        if (info->priv->url_base != NULL) {
                char *full_url;

                full_url = g_build_path ("/",
                                         info->priv->url_base,
                                         (const char *) prop,
                                         NULL);

                g_free (prop);

                return full_url;
        } else
                return prop;
}

/**
 * gupnp_service_info_get_service_type
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The UPnP service type, or NULL. g_free() after use.
 **/
char *
gupnp_service_info_get_service_type (GUPnPServiceInfo *info)
{
        return get_property (info, "serviceType");
}

/**
 * gupnp_service_info_get_id
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The ID, or NULL. g_free() after use.
 **/
char *
gupnp_service_info_get_id (GUPnPServiceInfo *info)
{
        return get_property (info, "serviceId");
}

/**
 * gupnp_service_info_get_scpd_url
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The SCPD URL, or NULL. g_free() after use.
 **/
char *
gupnp_service_info_get_scpd_url (GUPnPServiceInfo *info)
{
        return get_url_property (info, "SCPDURL");
}

/**
 * gupnp_service_info_get_control_url
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The control URL, or NULL. g_free() after use.
 **/
char *
gupnp_service_info_get_control_url (GUPnPServiceInfo *info)
{
        return get_url_property (info, "controlURL");
}

/**
 * gupnp_service_info_get_event_subscription_url
 * @info: A #GUPnPServiceInfo
 *
 * Return value: The event subscription URL, or NULL. g_free() after use.
 **/
char *
gupnp_service_info_get_event_subscription_url (GUPnPServiceInfo *info)
{
        return get_url_property (info, "eventSubURL");
}

/**
 * gupnp_service_info_request_introspection
 * @info: A #GUPnPServiceInfo
 *
 * Note that introspection object is created from the information in service
 * description document (SCPD) provided by the service so it can not be created
 * if the service does not provide an SCPD.
 *
 * Return value: A new #GUPnPServiceIntrospection for this service or NULL.
 * Unref after use.
 **/
GUPnPServiceIntrospection *
gupnp_service_info_get_introspection (GUPnPServiceInfo *info)
{
        GUPnPServiceIntrospection *introspection;
        char *scpd_url;

        scpd_url = gupnp_service_info_get_scpd_url (info);
        if (scpd_url == NULL)
                return NULL;

        introspection = gupnp_service_introspection_new (scpd_url);

        g_free (scpd_url);

        return introspection;
}
