#include "d3dUtil.h"

using namespace DirectX;
using namespace std::experimental;




// ------------------------------
// CreateTexture2DArray����
// ------------------------------
// ������������������������
// [In]d3dDevice			D3D�豸
// [In]d3dDeviceContext		D3D�豸������
// [In]srcTexVec			�������������
// [In]usage				D3D11_USAGEö��ֵ
// [In]bindFlags			D3D11_BIND_FLAGö��ֵ
// [In]cpuAccessFlags		D3D11_CPU_ACCESS_FLAGö��ֵ
// [In]miscFlags			D3D11_RESOURCE_MISC_FLAGö��ֵ
// [OutOpt]textureArray		���������������Դ
// [OutOpt]textureCubeView	���������������Դ��ͼ
static HRESULT CreateTexture2DArray(
	ID3D11Device * d3dDevice,
	ID3D11DeviceContext * d3dDeviceContext,
	std::vector<ID3D11Texture2D*>& srcTexVec,
	D3D11_USAGE usage,
	UINT bindFlags,
	UINT cpuAccessFlags,
	UINT miscFlags,
	ID3D11Texture2D** textureArray,
	ID3D11ShaderResourceView** textureArrayView);

//
// �������岿��
//

HRESULT CreateShaderFromFile(const WCHAR * csoFileNameInOut, const WCHAR * hlslFileName,
	LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob ** ppBlobOut)
{
	HRESULT hr = S_OK;

	// Ѱ���Ƿ����Ѿ�����õĶ�����ɫ��
	if (csoFileNameInOut && filesystem::exists(csoFileNameInOut))
	{
		return D3DReadFileToBlob(csoFileNameInOut, ppBlobOut);
	}
	else
	{
		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		// ���� D3DCOMPILE_DEBUG ��־���ڻ�ȡ��ɫ��������Ϣ���ñ�־���������������飬
		// ����Ȼ������ɫ�������Ż�����
		dwShaderFlags |= D3DCOMPILE_DEBUG;

		// ��Debug�����½����Ż��Ա������һЩ�����������
		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		ID3DBlob* errorBlob = nullptr;
		hr = D3DCompileFromFile(hlslFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, shaderModel,
			dwShaderFlags, 0, ppBlobOut, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob != nullptr)
			{
				OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			}
			SAFE_RELEASE(errorBlob);
			return hr;
		}

		// ��ָ��������ļ���������ɫ����������Ϣ���
		if (csoFileNameInOut)
		{
			return D3DWriteBlobToFile(*ppBlobOut, csoFileNameInOut, FALSE);
		}
	}

	return hr;
}

HRESULT CreateTexture2DArray(
	ID3D11Device * d3dDevice,
	ID3D11DeviceContext * d3dDeviceContext,
	std::vector<ID3D11Texture2D*>& srcTexVec,
	D3D11_USAGE usage,
	UINT bindFlags,
	UINT cpuAccessFlags,
	UINT miscFlags,
	ID3D11Texture2D** textureArray,
	ID3D11ShaderResourceView** textureArrayView)
{

	if (!textureArray && !textureArrayView || !d3dDevice || !d3dDeviceContext || srcTexVec.empty())
		return E_INVALIDARG;

	HRESULT hResult;
	UINT arraySize = (UINT)srcTexVec.size();
	bool generateMips = (bindFlags & D3D11_BIND_RENDER_TARGET) &&
		(miscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS);
	// ******************
	// ������������
	//

	D3D11_TEXTURE2D_DESC texDesc;
	srcTexVec[0]->GetDesc(&texDesc);

	D3D11_TEXTURE2D_DESC texArrayDesc;
	texArrayDesc.Width = texDesc.Width;
	texArrayDesc.Height = texDesc.Height;
	texArrayDesc.MipLevels = generateMips ? 0 : texDesc.MipLevels;
	texArrayDesc.ArraySize = arraySize;
	texArrayDesc.Format = texDesc.Format;
	texArrayDesc.SampleDesc.Count = 1;		// ����ʹ�ö��ز���
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = usage;
	texArrayDesc.BindFlags = bindFlags;
	texArrayDesc.CPUAccessFlags = cpuAccessFlags;
	texArrayDesc.MiscFlags = miscFlags;

	ID3D11Texture2D* texArray = nullptr;
	hResult = d3dDevice->CreateTexture2D(&texArrayDesc, nullptr, &texArray);
	if (FAILED(hResult))
	{
		return hResult;
	}

	texArray->GetDesc(&texArrayDesc);
	// ******************
	// �����е���������Դ��ֵ������������
	//

	UINT minMipLevels = (generateMips ? 1 : texArrayDesc.MipLevels);
	// ÿ������Ԫ��
	for (UINT i = 0; i < texArrayDesc.ArraySize; ++i)
	{
		// �����е�ÿ��mipmap�ȼ�
		for (UINT j = 0; j < minMipLevels; ++j)
		{
			D3D11_MAPPED_SUBRESOURCE mappedTex2D;
			// ����ӳ������i�����У�����j��mipmap�ȼ���2D����
			d3dDeviceContext->Map(srcTexVec[i],
				j, D3D11_MAP_READ, 0, &mappedTex2D);

			d3dDeviceContext->UpdateSubresource(
				texArray,
				D3D11CalcSubresource(j, i, texArrayDesc.MipLevels),	// i * mipLevel + j
				nullptr,
				mappedTex2D.pData,
				mappedTex2D.RowPitch,
				mappedTex2D.DepthPitch);
			// ֹͣӳ��
			d3dDeviceContext->Unmap(srcTexVec[i], j);
		}
	}

	// ******************
	// �������������SRV
	//
	if (textureArrayView)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = texArrayDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		viewDesc.Texture2DArray.MostDetailedMip = 0;
		viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
		viewDesc.Texture2DArray.FirstArraySlice = 0;
		viewDesc.Texture2DArray.ArraySize = arraySize;

		hResult = d3dDevice->CreateShaderResourceView(texArray, &viewDesc, textureArrayView);

		// ����mipmaps
		if (hResult == S_OK && generateMips)
		{
			d3dDeviceContext->GenerateMips(*textureArrayView);
		}
	}

	// ����Ƿ���Ҫ��������
	if (textureArray)
	{
		*textureArray = texArray;
	}
	else
	{
		SAFE_RELEASE(texArray);
	}

	return hResult;
}


HRESULT CreateDDSTexture2DArrayFromFile(
	ID3D11Device * d3dDevice,
	ID3D11DeviceContext * d3dDeviceContext,
	const std::vector<std::wstring>& fileNames,
	ID3D11Texture2D** textureArray,
	ID3D11ShaderResourceView** textureArrayView,
	bool generateMips)
{
	// ����豸����ɫ����Դ��ͼ���ļ��������Ƿ�ǿ�
	if (!d3dDevice || !textureArrayView || fileNames.empty())
		return E_INVALIDARG;

	HRESULT hResult;
	// ******************
	// ��ȡ��������
	//

	UINT arraySize = (UINT)fileNames.size();
	std::vector<ID3D11Texture2D*> srcTexVec(arraySize);
	std::vector<D3D11_TEXTURE2D_DESC> texDescVec(arraySize);
	for (UINT i = 0; i < arraySize; ++i)
	{
		// ������Щ���������ᱻGPUʹ�ã�����ʹ��D3D11_USAGE_STAGINGö��ֵ
		// ʹ��CPU���Զ�ȡ��Դ
		hResult = CreateDDSTextureFromFileEx(d3dDevice, d3dDeviceContext,
			fileNames[i].c_str(), 0, D3D11_USAGE_STAGING, 0,
			D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
			0, false, (ID3D11Resource**)&srcTexVec[i], nullptr);

		// ��ȡʧ�����ͷ�֮ǰ��ȡ������������
		if (FAILED(hResult))
		{
			for (UINT j = 0; j < i; ++j)
				SAFE_RELEASE(srcTexVec[j]);
			return hResult;
		}

		// ��ȡ�����õ�������Ϣ
		srcTexVec[i]->GetDesc(&texDescVec[i]);

		// ��Ҫ��������������mipLevels�����Ⱥ͸߶ȣ����ݸ�ʽ�Ƿ�һ�£�
		// ���������ݸ�ʽ��һ�µ��������ʹ��dxtex.exe(DirectX Texture Tool)
		// �����е�ͼƬת��һ�µ����ݸ�ʽ
		if (texDescVec[i].MipLevels != texDescVec[0].MipLevels || texDescVec[i].Width != texDescVec[0].Width ||
			texDescVec[i].Height != texDescVec[0].Height || texDescVec[i].Format != texDescVec[0].Format)
		{
			for (UINT j = 0; j < i; ++j)
				SAFE_RELEASE(srcTexVec[j]);
			return E_FAIL;
		}
	}

	hResult = CreateTexture2DArray(d3dDevice, d3dDeviceContext, srcTexVec,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | (generateMips ? D3D11_BIND_RENDER_TARGET : 0),
		0,
		(generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0),
		textureArray,
		textureArrayView);

	for (UINT i = 0; i < arraySize; ++i)
		SAFE_RELEASE(srcTexVec[i]);
	return hResult;
}

HRESULT CreateWICTexture2DArrayFromFile(
	ID3D11Device * d3dDevice,
	ID3D11DeviceContext * d3dDeviceContext,
	const std::vector<std::wstring>& fileNames,
	ID3D11Texture2D** textureArray,
	ID3D11ShaderResourceView** textureArrayView,
	bool generateMips)
{
	// ����豸����ɫ����Դ��ͼ���ļ��������Ƿ�ǿ�
	if (!d3dDevice || !textureArrayView || fileNames.empty())
		return E_INVALIDARG;

	HRESULT hResult;
	// ******************
	// ��ȡ��������
	//

	UINT arraySize = (UINT)fileNames.size();
	std::vector<ID3D11Texture2D*> srcTexVec(arraySize);
	std::vector<D3D11_TEXTURE2D_DESC> texDescVec(arraySize);
	for (UINT i = 0; i < arraySize; ++i)
	{
		// ������Щ���������ᱻGPUʹ�ã�����ʹ��D3D11_USAGE_STAGINGö��ֵ
		// ʹ��CPU���Զ�ȡ��Դ
		hResult = CreateWICTextureFromFileEx(d3dDevice, d3dDeviceContext,
			fileNames[i].c_str(), 0, D3D11_USAGE_STAGING, 0,
			D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
			0, WIC_LOADER_DEFAULT, (ID3D11Resource**)&srcTexVec[i], nullptr);

		// ��ȡʧ�����ͷ�֮ǰ��ȡ������������
		if (FAILED(hResult))
		{
			for (UINT j = 0; j < i; ++j)
				SAFE_RELEASE(srcTexVec[j]);
			return hResult;
		}

		// ��ȡ�����õ�������Ϣ
		srcTexVec[i]->GetDesc(&texDescVec[i]);

		// ��Ҫ��������������mipLevels�����Ⱥ͸߶ȣ����ݸ�ʽ�Ƿ�һ�£�
		// ���������ݸ�ʽ��һ�µ��������ʹ��ͼ��������ͳһ������
		// �����е�ͼƬת��һ�µ����ݸ�ʽ
		if (texDescVec[i].MipLevels != texDescVec[0].MipLevels || texDescVec[i].Width != texDescVec[0].Width ||
			texDescVec[i].Height != texDescVec[0].Height || texDescVec[i].Format != texDescVec[0].Format)
		{
			for (UINT j = 0; j < i; ++j)
				SAFE_RELEASE(srcTexVec[j]);
			return E_FAIL;
		}
	}

	hResult = CreateTexture2DArray(d3dDevice, d3dDeviceContext, srcTexVec,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | (generateMips ? D3D11_BIND_RENDER_TARGET : 0),
		0,
		(generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0),
		nullptr,
		textureArrayView);

	for (UINT i = 0; i < arraySize; ++i)
		SAFE_RELEASE(srcTexVec[i]);
	return hResult;
}

HRESULT CreateWICTexture2DCubeFromFile(
	ID3D11Device * d3dDevice,
	ID3D11DeviceContext * d3dDeviceContext,
	const std::wstring & cubeMapFileName,
	ID3D11Texture2D ** textureArray,
	ID3D11ShaderResourceView ** textureCubeView,
	bool generateMips)
{
	// ����豸���豸�������Ƿ�ǿ�
	// ���������������������ͼֻҪ������һ���ǿռ���
	if (!d3dDevice || !d3dDeviceContext || !(textureArray || textureCubeView))
		return E_INVALIDARG;

	// ******************
	// ��ȡ��պ�����
	//

	ID3D11Texture2D* srcTex = nullptr;
	ID3D11ShaderResourceView* srcTexSRV = nullptr;

	// ����Դ����GPU����
	HRESULT hResult = CreateWICTextureFromFileEx(d3dDevice,
		d3dDeviceContext,
		cubeMapFileName.c_str(),
		0,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | (generateMips ? D3D11_BIND_RENDER_TARGET : 0),
		0,
		(generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0),
		WIC_LOADER_DEFAULT,
		(ID3D11Resource**)&srcTex,
		(generateMips ? &srcTexSRV : nullptr));

	// �ļ�δ��
	if (FAILED(hResult))
	{
		return hResult;
	}

	D3D11_TEXTURE2D_DESC texDesc, texArrayDesc;
	srcTex->GetDesc(&texDesc);

	// Ҫ����߱�4:3
	if (texDesc.Width * 3 != texDesc.Height * 4)
	{
		SAFE_RELEASE(srcTex);
		SAFE_RELEASE(srcTexSRV);
		return E_FAIL;
	}

	// (��ѡ)����mipmap��
	if (generateMips)
	{
		d3dDeviceContext->GenerateMips(srcTexSRV);
	}


	// ******************
	// ��������6������������
	//

	UINT squareLength = texDesc.Width / 4;
	texArrayDesc.Width = squareLength;
	texArrayDesc.Height = squareLength;
	texArrayDesc.MipLevels = (generateMips ? 0 : 1);	// ָ��0������½���������mipmap��
	texArrayDesc.ArraySize = 6;
	texArrayDesc.Format = texDesc.Format;
	texArrayDesc.SampleDesc.Count = 1;
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;
	texArrayDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;	// �������д���TextureCube

	ID3D11Texture2D* texArray = nullptr;
	hResult = d3dDevice->CreateTexture2D(&texArrayDesc, nullptr, &texArray);
	if (FAILED(hResult))
	{
		SAFE_RELEASE(srcTex);
		SAFE_RELEASE(srcTexSRV);
		return hResult;
	}

	// ******************
	// ѡȡԭ��պ�������6�������������򣬿�������������
	//

	D3D11_BOX box;
	// box����������: 
	//    front
	//   / 
	//  /_____right
	//  |
	//  |
	//  bottom
	box.front = 0;
	box.back = 1;

	for (UINT i = 0; i < texArrayDesc.MipLevels; ++i)
	{
		// +X�濽��
		box.left = squareLength * 2;
		box.top = squareLength;
		box.right = squareLength * 3;
		box.bottom = squareLength * 2;
		d3dDeviceContext->CopySubresourceRegion(
			texArray,
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_POSITIVE_X, texArrayDesc.MipLevels),
			0, 0, 0,
			srcTex,
			i,
			&box);

		// -X�濽��
		box.left = 0;
		box.top = squareLength;
		box.right = squareLength;
		box.bottom = squareLength * 2;
		d3dDeviceContext->CopySubresourceRegion(
			texArray,
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_NEGATIVE_X, texArrayDesc.MipLevels),
			0, 0, 0,
			srcTex,
			i,
			&box);

		// +Y�濽��
		box.left = squareLength;
		box.top = 0;
		box.right = squareLength * 2;
		box.bottom = squareLength;
		d3dDeviceContext->CopySubresourceRegion(
			texArray,
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_POSITIVE_Y, texArrayDesc.MipLevels),
			0, 0, 0,
			srcTex,
			i,
			&box);


		// -Y�濽��
		box.left = squareLength;
		box.top = squareLength * 2;
		box.right = squareLength * 2;
		box.bottom = squareLength * 3;
		d3dDeviceContext->CopySubresourceRegion(
			texArray,
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_NEGATIVE_Y, texArrayDesc.MipLevels),
			0, 0, 0,
			srcTex,
			i,
			&box);

		// +Z�濽��
		box.left = squareLength;
		box.top = squareLength;
		box.right = squareLength * 2;
		box.bottom = squareLength * 2;
		d3dDeviceContext->CopySubresourceRegion(
			texArray,
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_POSITIVE_Z, texArrayDesc.MipLevels),
			0, 0, 0,
			srcTex,
			i,
			&box);

		// -Z�濽��
		box.left = squareLength * 3;
		box.top = squareLength;
		box.right = squareLength * 4;
		box.bottom = squareLength * 2;
		d3dDeviceContext->CopySubresourceRegion(
			texArray,
			D3D11CalcSubresource(i, D3D11_TEXTURECUBE_FACE_NEGATIVE_Z, texArrayDesc.MipLevels),
			0, 0, 0,
			srcTex,
			i,
			&box);

		// ��һ��mipLevel���������߶���ԭ����1/2
		squareLength /= 2;
	}


	// ******************
	// ����������������SRV
	//
	if (textureCubeView)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = texArrayDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		viewDesc.TextureCube.MostDetailedMip = 0;
		viewDesc.TextureCube.MipLevels = texArrayDesc.MipLevels;

		hResult = d3dDevice->CreateShaderResourceView(texArray, &viewDesc, textureCubeView);
	}

	// ����Ƿ���Ҫ��������
	if (textureArray)
	{
		*textureArray = texArray;
	}
	else
	{
		SAFE_RELEASE(texArray);
	}

	SAFE_RELEASE(srcTex);
	SAFE_RELEASE(srcTexSRV);

	return hResult;
}

HRESULT CreateWICTexture2DCubeFromFile(
	ID3D11Device * d3dDevice,
	ID3D11DeviceContext * d3dDeviceContext,
	const std::vector<std::wstring>& cubeMapFileNames,
	ID3D11Texture2D ** textureArray,
	ID3D11ShaderResourceView ** textureCubeView,
	bool generateMips)
{
	// ����豸���豸�������Ƿ�ǿ�
	// �ļ�����Ŀ��Ҫ��С��6
	// �����������Դ��ͼֻҪ������һ���ǿռ���
	UINT arraySize = (UINT)cubeMapFileNames.size();

	if (!d3dDevice || !d3dDeviceContext || arraySize < 6 || !(textureArray || textureCubeView))
		return E_INVALIDARG;

	// ******************
	// ��ȡ����
	//

	HRESULT hResult;
	std::vector<ID3D11Texture2D*> srcTexVec(arraySize, nullptr);
	std::vector<ID3D11ShaderResourceView*> srcTexSRVVec(arraySize, nullptr);
	std::vector<D3D11_TEXTURE2D_DESC> texDescVec(arraySize);

	for (UINT i = 0; i < arraySize; ++i)
	{
		// ����Դ����GPU����
		hResult = CreateWICTextureFromFileEx(d3dDevice,
			d3dDeviceContext,
			cubeMapFileNames[i].c_str(),
			0,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE | (generateMips ? D3D11_BIND_RENDER_TARGET : 0),
			0,
			(generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0),
			WIC_LOADER_DEFAULT,
			(ID3D11Resource**)&srcTexVec[i],
			(generateMips ? &srcTexSRVVec[i] : nullptr));

		// ��ȡ�����õ�������Ϣ
		srcTexVec[i]->GetDesc(&texDescVec[i]);

		// ��Ҫ��������������mipLevels�����Ⱥ͸߶ȣ����ݸ�ʽ�Ƿ�һ�£�
		// ���������ݸ�ʽ��һ�µ��������ʹ��dxtex.exe(DirectX Texture Tool)
		// �����е�ͼƬת��һ�µ����ݸ�ʽ
		if (texDescVec[i].MipLevels != texDescVec[0].MipLevels || texDescVec[i].Width != texDescVec[0].Width ||
			texDescVec[i].Height != texDescVec[0].Height || texDescVec[i].Format != texDescVec[0].Format)
		{
			for (UINT j = 0; j < i; ++j)
			{
				SAFE_RELEASE(srcTexVec[j]);
				SAFE_RELEASE(srcTexSRVVec[j]);
			}
			return E_FAIL;
		}
	}

	if (generateMips)
	{
		// (��ѡ)����mipmap��
		for (UINT i = 0; i < arraySize; ++i)
			d3dDeviceContext->GenerateMips(srcTexSRVVec[i]);
	}

	// ******************
	// ��������������
	//

	// ******************
	// ������������
	//
	D3D11_TEXTURE2D_DESC texArrayDesc;
	texArrayDesc.Width = texDescVec[0].Width;
	texArrayDesc.Height = texDescVec[0].Height;
	texArrayDesc.MipLevels = (generateMips ? 0 : 1);	// ָ��0������½���������mipmap��
	texArrayDesc.ArraySize = arraySize;
	texArrayDesc.Format = texDescVec[0].Format;
	texArrayDesc.SampleDesc.Count = 1;
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;
	texArrayDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;	// �������д���TextureCube

	ID3D11Texture2D* texArray = nullptr;
	hResult = d3dDevice->CreateTexture2D(&texArrayDesc, nullptr, &texArray);

	if (FAILED(hResult))
	{
		for (UINT i = 0; i < arraySize; ++i)
		{
			SAFE_RELEASE(srcTexVec[i]);
			SAFE_RELEASE(srcTexSRVVec[i]);
		}

		return hResult;
	}

	// ******************
	// ��ԭ��������������Դ��������������
	//
	texArray->GetDesc(&texArrayDesc);

	for (UINT i = 0; i < arraySize; ++i)
	{
		for (UINT j = 0; j < texArrayDesc.MipLevels; ++j)
		{
			d3dDeviceContext->CopySubresourceRegion(
				texArray,
				D3D11CalcSubresource(j, i, texArrayDesc.MipLevels),
				0, 0, 0,
				srcTexVec[i],
				j,
				nullptr);
		}
	}

	// ******************
	// ����������������SRV
	//
	if (textureCubeView)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = texArrayDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		viewDesc.TextureCube.MostDetailedMip = 0;
		viewDesc.TextureCube.MipLevels = texArrayDesc.MipLevels;

		hResult = d3dDevice->CreateShaderResourceView(texArray, &viewDesc, textureCubeView);
	}

	// ����Ƿ���Ҫ��������
	if (textureArray)
	{
		*textureArray = texArray;
	}
	else
	{
		SAFE_RELEASE(texArray);
	}

	// �ͷ�������Դ
	for (UINT i = 0; i < arraySize; ++i)
	{
		SAFE_RELEASE(srcTexVec[i]);
		SAFE_RELEASE(srcTexSRVVec[i]);
	}

	return hResult;
}