/* 
 * Copyright (C) 2006 OpenedHand Ltd.
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

#include <string.h>

#include "gupnp-device-proxy.h"
#include "gupnp-device-proxy-private.h"
#include "xml-util.h"

static void
gupnp_device_proxy_info_init (GUPnPDeviceInfoIface *iface);

G_DEFINE_TYPE_EXTENDED (GUPnPDeviceProxy,
                        gupnp_device_proxy,
                        G_TYPE_OBJECT,
                        0,
                        G_IMPLEMENT_INTERFACE (GUPNP_TYPE_DEVICE_INFO,
                                               gupnp_device_proxy_info_init));

struct _GUPnPDeviceProxyPrivate {
        char *location;

        xmlDoc *doc;

        xmlNode *element;
};

static const char *
gupnp_device_proxy_get_location (GUPnPDeviceInfo *info)
{
        GUPnPDeviceProxy *proxy;
        
        proxy = GUPNP_DEVICE_PROXY (info);
        
        return proxy->priv->location;
}

static xmlNode *
gupnp_device_proxy_get_element (GUPnPDeviceInfo *info)
{
        GUPnPDeviceProxy *proxy;
        
        proxy = GUPNP_DEVICE_PROXY (info);

        return proxy->priv->element;
}

static void
gupnp_device_proxy_init (GUPnPDeviceProxy *proxy)
{
        proxy->priv = G_TYPE_INSTANCE_GET_PRIVATE (proxy,
                                                   GUPNP_TYPE_DEVICE_PROXY,
                                                   GUPnPDeviceProxyPrivate);
}

static void
gupnp_device_proxy_finalize (GObject *object)
{
        GUPnPDeviceProxy *proxy;

        proxy = GUPNP_DEVICE_PROXY (object);

        g_free (proxy->priv->location);

        if (proxy->priv->doc)
                xmlFreeDoc (proxy->priv->doc);
}

static void
gupnp_device_proxy_class_init (GUPnPDeviceProxyClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gupnp_device_proxy_finalize;
        
        g_type_class_add_private (klass, sizeof (GUPnPDeviceProxyPrivate));
}

static void
gupnp_device_proxy_info_init (GUPnPDeviceInfoIface *iface)
{
        iface->get_location = gupnp_device_proxy_get_location;
        iface->get_element  = gupnp_device_proxy_get_element;
}

/**
 * gupnp_device_proxy_list_devices
 * @proxy: A #GUPnPDeviceProxy
 *
 * Return value: A #GList of #GUPnPDeviceProxy objects representing the
 * devices directly contained in @proxy. The returned list should be
 * g_list_free()'ed and the elements should be g_object_unref()'ed.
 **/
GList *
gupnp_device_proxy_list_devices (GUPnPDeviceProxy *proxy)
{
        GList *devices;
        xmlNode *element;

        g_return_val_if_fail (GUPNP_IS_DEVICE_PROXY (proxy), NULL);

        devices = NULL;

        element = xml_util_get_element (proxy->priv->element,
                                        "deviceList",
                                        NULL);
        if (!element)
                return NULL;

        for (element = element->children; element; element = element->next) {
                if (!strcmp ("device", (char *) element->name)) {
                        GUPnPDeviceProxy *child;

                        child = _gupnp_device_proxy_new_from_element
                                        (proxy->priv->location, element);

                        devices = g_list_prepend (devices, child);
                }
        }

        return devices;
}

GList *
gupnp_device_proxy_list_device_types (GUPnPDeviceProxy *proxy)
{
        g_return_val_if_fail (GUPNP_IS_DEVICE_PROXY (proxy), NULL);

        return NULL;
}

GUPnPDeviceProxy *
gupnp_device_proxy_get_device (GUPnPDeviceProxy *proxy,
                               const char       *type)
{
        g_return_val_if_fail (GUPNP_IS_DEVICE_PROXY (proxy), NULL);
        g_return_val_if_fail (type, NULL);

        return NULL;
}

GList *
gupnp_device_proxy_list_services (GUPnPDeviceProxy *proxy)
{
        g_return_val_if_fail (GUPNP_IS_DEVICE_PROXY (proxy), NULL);

        return NULL;
}

GList *
gupnp_device_proxy_list_service_types (GUPnPDeviceProxy *proxy)
{
        g_return_val_if_fail (GUPNP_IS_DEVICE_PROXY (proxy), NULL);

        return NULL;
}

GUPnPServiceProxy *
gupnp_device_proxy_get_service (GUPnPDeviceProxy *proxy,
                                const char       *type)
{
        g_return_val_if_fail (GUPNP_IS_DEVICE_PROXY (proxy), NULL);
        g_return_val_if_fail (type, NULL);

        return NULL;
}

static xmlNode *
find_device_element_by_udn (xmlNode    *element,
                            const char *udn)
{
        xmlNode *tmp;

        tmp = xml_util_get_element (element,
                                    "UDN",
                                    NULL);
        if (tmp) {
                xmlChar *content;
                gboolean match;

                content = xmlNodeGetContent (tmp);
                
                match = !strcmp (udn, (char *) content);

                xmlFree (content);

                if (match)
                        return element;
        }

        tmp = xml_util_get_element (element,
                                    "deviceList",
                                    NULL);
        if (tmp) {
                for (tmp = tmp->children; tmp; tmp = tmp->next) {
                        element = find_device_element_by_udn (tmp, udn);
                        if (element)
                                return element;
                }
        }

        return NULL;
}

GUPnPDeviceProxy *
_gupnp_device_proxy_new_from_udn (const char *location,
                                  const char *udn)
{
        GUPnPDeviceProxy *proxy;

        g_return_val_if_fail (location, NULL);
        g_return_val_if_fail (udn, NULL);

        proxy = g_object_new (GUPNP_TYPE_DEVICE_PROXY,
                              NULL);

        proxy->priv->location = g_strdup (location);

        proxy->priv->doc = xmlParseFile (proxy->priv->location);

        proxy->priv->element =
                xml_util_get_element ((xmlNode *) proxy->priv->doc,
                                      "root",
                                      "device",
                                      NULL);
        proxy->priv->element =
                find_device_element_by_udn (proxy->priv->element, udn);

        if (!proxy->priv->element) {
                g_warning ("Device description does not contain device "
                           "with UDN \"%s\".", udn);

                g_object_unref (proxy);
                proxy = NULL;
        }

        return proxy;
}

GUPnPDeviceProxy *
_gupnp_device_proxy_new_from_element (const char *location,
                                      xmlNode    *element)
{
        GUPnPDeviceProxy *proxy;

        g_return_val_if_fail (location, NULL);
        g_return_val_if_fail (element, NULL);

        proxy = g_object_new (GUPNP_TYPE_DEVICE_PROXY,
                              NULL);

        proxy->priv->location = g_strdup (location);

        proxy->priv->element = element;

        return proxy;
}