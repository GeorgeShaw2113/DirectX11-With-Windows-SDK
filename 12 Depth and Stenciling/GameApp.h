#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <d3dcompiler.h>
#include <directxmath.h>
#include <DirectXColors.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include "d3dApp.h"
#include "LightHelper.h"
#include "Geometry.h"
#include "Camera.h"
#include "RenderStates.h"

class GameApp : public D3DApp
{
public:

	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
		DirectX::XMMATRIX texTransform;
		Material material;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT4 eyePos;

	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};


	struct CBNeverChange
	{
		DirectX::XMMATRIX reflection;
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;		// �����֤16�ֽڶ���
	};

	// һ��������С����Ϸ������
	class GameObject
	{
	public:
		// ��ȡλ��
		DirectX::XMFLOAT3 GetPosition() const;

		// ���û�����
		void SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData& meshData);
		// ��������
		void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);
		// ���ò���
		void SetMaterial(const Material& material);
		// ���þ���
		void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
		void SetWorldMatrix(DirectX::FXMMATRIX world);
		void SetTexTransformMatrix(const DirectX::XMFLOAT4X4& texTransform);
		void SetTexTransformMatrix(DirectX::FXMMATRIX texTransform);
		// ����
		void Draw(ComPtr<ID3D11DeviceContext> deviceContext);
	private:
		DirectX::XMFLOAT4X4 mWorldMatrix;				// �������
		DirectX::XMFLOAT4X4 mTexTransform;				// �����任����
		Material mMaterial;								// �������
		ComPtr<ID3D11ShaderResourceView> mTexture;		// ����
		ComPtr<ID3D11Buffer> mVertexBuffer;				// ���㻺����
		ComPtr<ID3D11Buffer> mIndexBuffer;				// ����������
		int mIndexCount;								// ������Ŀ	
	};

	// �����ģʽ
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
	
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	// objFileNameInOutΪ����õ���ɫ���������ļ�(.*so)������ָ��������Ѱ�Ҹ��ļ�����ȡ
	// hlslFileNameΪ��ɫ�����룬��δ�ҵ���ɫ���������ļ��������ɫ������
	// ����ɹ�����ָ����objFileNameInOut���򱣴����õ���ɫ����������Ϣ�����ļ�
	// ppBlobOut�����ɫ����������Ϣ
	HRESULT CreateShaderFromFile(const WCHAR* objFileNameInOut, const WCHAR* hlslFileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** ppBlobOut);

private:
	bool InitEffect();
	bool InitResource();

private:
	
	ComPtr<ID2D1SolidColorBrush> mColorBrush;				// ��ɫ��ˢ
	ComPtr<IDWriteFont> mFont;								// ����
	ComPtr<IDWriteTextFormat> mTextFormat;					// �ı���ʽ

	ComPtr<ID3D11InputLayout> mVertexLayout2D;				// ����2D�Ķ������벼��
	ComPtr<ID3D11InputLayout> mVertexLayout3D;				// ����3D�Ķ������벼��
	ComPtr<ID3D11Buffer> mConstantBuffers[5];				// ����������

	GameObject mWireFence;									// ��ʺ�
	GameObject mFloor;										// �ذ�
	std::vector<GameObject> mWalls;							// ǽ��
	GameObject mWater;										// ˮ
	GameObject mMirror;										// ����

	ComPtr<ID3D11VertexShader> mVertexShader3D;				// ����3D�Ķ�����ɫ��
	ComPtr<ID3D11PixelShader> mPixelShader3D;				// ����3D��������ɫ��
	ComPtr<ID3D11VertexShader> mVertexShader2D;				// ����2D�Ķ�����ɫ��
	ComPtr<ID3D11PixelShader> mPixelShader2D;				// ����2D��������ɫ��

	CBChangesEveryFrame mCBFrame;							// �û�������Ž���ÿһ֡���и��µı���
	CBChangesOnResize mCBOnReSize;							// �û�������Ž��ڴ��ڴ�С�仯ʱ���µı���
	CBNeverChange mCBNeverChange;							// �û�������Ų����ٽ����޸ĵı���

	ComPtr<ID3D11SamplerState> mSamplerState;				// ������״̬

	std::shared_ptr<Camera> mCamera;						// �����
	CameraMode mCameraMode;									// �����ģʽ

};


#endif