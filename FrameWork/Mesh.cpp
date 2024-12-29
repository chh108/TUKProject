//-----------------------------------------------------------------------------
// File: Mesh.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Mesh.h"
#include "Object.h"
#include "DebugLog.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CMeshFromFbx::CMeshFromFbx(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, int nVertices, int nIndices, int *pnIndices, XMFLOAT2* pxmf2UVs)
{
	m_nVertices = nVertices;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	if (!pxmf2UVs) {
		debugLog << "pxmf2UVs is NULL!" << std::endl;
		return;
	}

	// 1. Position Buffer 20241229  UV Buffer 수정
	m_pd3dPositionBuffer = ::CreateBufferResource(
		pd3dDevice, pd3dCommandList, NULL, 
		sizeof(XMFLOAT4) * m_nVertices, 
		D3D12_HEAP_TYPE_UPLOAD, 
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	if (!m_pd3dPositionBuffer) {
		debugLog << "Failed to create Position buffer!" << std::endl;
		return;
	}

	HRESULT hr = m_pd3dPositionBuffer->Map(0, NULL, (void**)&m_pxmf4MappedPositions);
	if (FAILED(hr)) {
		debugLog << "Failed to map Position buffer! HRESULT: " << hr << std::endl;
		return;
	}

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT4);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT4) * m_nVertices;

	// 2. UV Buffer 생성
	m_pd3dUVBuffer = ::CreateBufferResource(
		pd3dDevice, pd3dCommandList, pxmf2UVs,
		sizeof(XMFLOAT2) * m_nVertices,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	if (!m_pd3dUVBuffer) {
		debugLog << "Failed to create UV buffer!" << std::endl;
		return;
	}

	//UV Mapping
	hr = m_pd3dUVBuffer->Map(0, NULL, (void**)&m_pxmf2MappedUVs);
	if (FAILED(hr)) {
		debugLog << "Failed to map UV buffer! HRESULT: " << hr << std::endl;
		return;
	}

	memcpy(m_pxmf2MappedUVs, pxmf2UVs, sizeof(XMFLOAT2) * m_nVertices);
	m_pd3dUVBuffer->Unmap(0, NULL);

	m_d3dUVBufferView.BufferLocation = m_pd3dUVBuffer->GetGPUVirtualAddress();
	m_d3dUVBufferView.StrideInBytes = sizeof(XMFLOAT2);
	m_d3dUVBufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;

	//for (int i = 0; i < m_nVertices; i++) {
	//	debugLog << "UV[" << i << "]: (" << m_pxmf2MappedUVs[i].x << ", " << m_pxmf2MappedUVs[i].y << ")" << std::endl;
	//}

	// 3. Index Buffer

	m_nIndices = nIndices;
	m_pd3dIndexBuffer = ::CreateBufferResource(
		pd3dDevice, pd3dCommandList, pnIndices, 
		sizeof(UINT) * m_nIndices, 
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);

	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
}

CMeshFromFbx::~CMeshFromFbx()
{
	if (m_pd3dPositionBuffer) m_pd3dPositionBuffer->Release();
	if (m_pd3dUVBuffer) m_pd3dUVBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
}

void CMeshFromFbx::ReleaseUploadBuffers()
{
	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
	m_pd3dIndexUploadBuffer = NULL;
}

void CMeshFromFbx::OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	// 4. Vertex Buffers 설정 (Position + UV)
	D3D12_VERTEX_BUFFER_VIEW pVertexBufferViews[2] = { m_d3dPositionBufferView, m_d3dUVBufferView };
	pd3dCommandList->IASetVertexBuffers(0, 2, pVertexBufferViews);
}

void CMeshFromFbx::Render(ID3D12GraphicsCommandList *pd3dCommandList)
{
	OnPrepareRender(pd3dCommandList, NULL);
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
	pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

