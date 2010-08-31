/*
 * IDirect3DVertexShader9 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/* IDirect3DVertexShader9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVertexShader9Impl_QueryInterface(LPDIRECT3DVERTEXSHADER9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexShader9)) {
        IDirect3DVertexShader9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexShader9Impl_AddRef(LPDIRECT3DVERTEXSHADER9 iface) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        IWineD3DVertexShader_AddRef(This->wineD3DVertexShader);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DVertexShader9Impl_Release(LPDIRECT3DVERTEXSHADER9 iface) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        IWineD3DVertexShader_Release(This->wineD3DVertexShader);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DVertexShader9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexShader9Impl_GetDevice(IDirect3DVertexShader9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVertexShader9Impl_GetFunction(LPDIRECT3DVERTEXSHADER9 iface, VOID* pData, UINT* pSizeOfData) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, data %p, data_size %p.\n", iface, pData, pSizeOfData);

    wined3d_mutex_lock();
    hr = IWineD3DVertexShader_GetFunction(This->wineD3DVertexShader, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DVertexShader9Vtbl Direct3DVertexShader9_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexShader9Impl_QueryInterface,
    IDirect3DVertexShader9Impl_AddRef,
    IDirect3DVertexShader9Impl_Release,
    /* IDirect3DVertexShader9 */
    IDirect3DVertexShader9Impl_GetDevice,
    IDirect3DVertexShader9Impl_GetFunction
};

static void STDMETHODCALLTYPE d3d9_vertexshader_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_vertexshader_wined3d_parent_ops =
{
    d3d9_vertexshader_wined3d_object_destroyed,
};

HRESULT vertexshader_init(IDirect3DVertexShader9Impl *shader, IDirect3DDevice9Impl *device, const DWORD *byte_code)
{
    HRESULT hr;

    shader->ref = 1;
    shader->lpVtbl = &Direct3DVertexShader9_Vtbl;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateVertexShader(device->WineD3DDevice, byte_code, NULL,
            shader, &d3d9_vertexshader_wined3d_parent_ops, &shader->wineD3DVertexShader);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex shader, hr %#x.\n", hr);
        return hr;
    }

    shader->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(shader->parentDevice);

    return D3D_OK;
}
