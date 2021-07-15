//
//  main.cpp
//  VimToDatasmith
//
//  Created by Richard Young on 2021-05-17.
//

#include "CVimToDatasmith.h"

#include "DebugTools.h"

DISABLE_SDK_WARNINGS_START

	#include "DatasmithExporterManager.h"
	#include "DatasmithSceneExporter.h"
	#include "DatasmithSceneFactory.h"
	#include "DatasmithMeshExporter.h"
	#include "DatasmithMesh.h"

DISABLE_SDK_WARNINGS_END

#include <iostream>


BeginVim2DatasmithNameSpace


enum ESpecialValues {
	kNoGeometry = -1,
	kNoInstance = -1,
	kInvalidIndex = -1,
};


// Constructor
CVimToDatasmith::MaterialEntry::MaterialEntry(int32_t inVimId)
:	mCount(0)
,	mVimId(inVimId)
,	mMaterialElement(FDatasmithSceneFactory::CreateUEPbrMaterial(UTF8_TO_TCHAR(Utf8StringFormat("%d", inVimId).c_str())))
{
}


// Copy constructor required to be in a std::vector
CVimToDatasmith::MaterialEntry::MaterialEntry(const MaterialEntry& inOther)
:	mCount((int32_t)inOther.mCount)
,	mVimId(inOther.mVimId)
,	mMaterialElement(inOther.mMaterialElement)
,	mColor(inOther.mColor)
,	mParams(inOther.mParams)
{
}


// Constructor
CVimToDatasmith::CVimToDatasmith()
{
	mGeometryToDatasmithMeshMap[kNoGeometry] = TSharedPtr<IDatasmithMeshElement>();
}


// Destructor
CVimToDatasmith::~CVimToDatasmith()
{
}


// Parse parameters to get Vim file path and datasmith file path
void CVimToDatasmith::GetParameters(int argc, const utf8_t* argv[])
{
	if (argc < 2 || argc > 3)
		Usage();
	
	// Process input vim file argument
	mVimFilePath = argv[1];
	
	std::string vimPath;
	std::string vimName;
	std::string vimExtension;
	ExtractPathNameExtension(mVimFilePath, &vimPath, &vimName, &vimExtension);
	
	if (vimExtension != ".vim")
		ThrowMessage("Invalid vim file name \"%s\"", mVimFilePath.c_str());
	
	// Process output datassmith file argument
	if (argc > 2)
	{
		std::string datasmithExtension;
		ExtractPathNameExtension(argv[2], &mDatasmithFolderPath, &mDatasmithFileName, &datasmithExtension);
		if (datasmithExtension.size() != 0 && datasmithExtension != ".udatasmith")
			ThrowMessage("Invalid datasmith file name \"%s\"", argv[2]);
		if (mDatasmithFileName.size() == 0)
			mDatasmithFileName = vimName;
	}
	else
	{
		mDatasmithFolderPath = vimPath;
		mDatasmithFileName = vimName;
	}

	mOutputPath = UTF8_TO_TCHAR((mDatasmithFolderPath + "/" + mDatasmithFileName + "_Assets").c_str());
	
	DebugF("Convert \"%s\" -> \"%s\"\n", mVimFilePath.c_str(), (mDatasmithFolderPath + "/" + mDatasmithFileName + ".udatasmith").c_str());
}


// Initialize the Vim scene with the vim file
void CVimToDatasmith::ReadVimFile()
{
	Vim::VimErrorCodes vimReadResult = mVimScene.ReadFile(mVimFilePath);
	if (vimReadResult != Vim::VimErrorCodes::Success)
		ThrowMessage("CVimToDatasmith::ReadVimFile - ReadFile return error %d", vimReadResult);
}


// Create datasmith materials from Vim ones
void CVimToDatasmith::CreateMaterials()
{
	TraceF("CVimToDatasmith::CreateMaterials\n");
	Vim::EntityTable& materialTable = mVimScene.mEntityTables["table:Rvt.Material"];
	const std::vector<double>& idArray = materialTable.mNumericColumns["Id"];
	const std::vector<double>& colorXArray = materialTable.mNumericColumns["Color.X"];
	const std::vector<double>& colorYArray = materialTable.mNumericColumns["Color.Y"];
	const std::vector<double>& colorZArray = materialTable.mNumericColumns["Color.Z"];
	const std::vector<double>& transparencyArray = materialTable.mNumericColumns["Transparency"];
	const std::vector<double>& glossinessArray = materialTable.mNumericColumns["Glossiness"];
	const std::vector<double>& smoothnessArray = materialTable.mNumericColumns["Smoothness"];

	for (size_t i = 0; i < idArray.size(); i++)
	{
		uint32_t vimMaterialId = (uint32_t)idArray[i];

		VerboseF("CVimToDatasmith::CreateMaterials - MaterialId = %u\n", vimMaterialId);
		
		TestAssert(mVimToDatasmithMaterialMap.find(vimMaterialId) == mVimToDatasmithMaterialMap.end());
		mVimToDatasmithMaterialMap[vimMaterialId] = mMaterials.size();

		CVimToDatasmith::MaterialEntry materialEntry(vimMaterialId);
		
		materialEntry.mColor.x = colorXArray.size() > i ? (float)colorXArray[i] : 1.0f;
		materialEntry.mColor.y = colorYArray.size() > i ? (float)colorYArray[i] : 1.0f;
		materialEntry.mColor.z = colorZArray.size() > i ? (float)colorZArray[i] : 1.0f;
		materialEntry.mColor.w = transparencyArray.size() > i ? 1.0f - (float)transparencyArray[i] : 1.0f;

		materialEntry.mParams.x = glossinessArray.size() > i ? (float)glossinessArray[i] / 256.0f : 0.5f;
		materialEntry.mParams.y = smoothnessArray.size() > i ? (float)smoothnessArray[i] / 256.0f : 50.0f / 256.0f;

		IDatasmithUEPbrMaterialElement& element = materialEntry.mMaterialElement.Get();
		element.SetLabel(UTF8_TO_TCHAR(Utf8StringFormat("Vim %d", vimMaterialId).c_str()));
		IDatasmithMaterialExpressionColor* DiffuseExpression = element.AddMaterialExpression< IDatasmithMaterialExpressionColor >();
		if (DiffuseExpression != nullptr)
		{
			FLinearColor dsColor(materialEntry.mColor.x, materialEntry.mColor.y, materialEntry.mColor.z, materialEntry.mColor.w);
			DiffuseExpression->GetColor() = dsColor;
			DiffuseExpression->SetName(TEXT("Base Color"));
			DiffuseExpression->ConnectExpression(element.GetBaseColor());
			if (materialEntry.mColor.w < 0.999)
				DiffuseExpression->ConnectExpression(element.GetOpacity(), 3);
		}
		// material.mColor.w = transparencyArray.size() > i ? 1.0f - (float)transparencyArray[i] : 1.0f;
		// material.mParams.x = glossinessArray.size() > i ? (float)glossinessArray[i] / 256.0f : 0.5f;
		// material.mParams.y = smoothnessArray.size() > i ? (float)smoothnessArray[i] / 256.0f : 50.0f / 256.0f;
		
		mMaterials.push_back(materialEntry);
	}
}


// Return (and create if necessary) the mesh for the specified geometry
TSharedPtr<IDatasmithMeshElement> CVimToDatasmith::GetDatasmithMesh(int32 geometryIndex)
{
	GeometryToDatasmithMeshMap::iterator iter = mGeometryToDatasmithMeshMap.find(geometryIndex);
	if (iter != mGeometryToDatasmithMeshMap.end())
		return iter->second;

	TSharedPtr<IDatasmithMeshElement> meshElement = CreateDatasmithMesh(geometryIndex);
	mGeometryToDatasmithMeshMap[geometryIndex] = meshElement;
	mDatasmithScene->AddMesh(meshElement);
	return meshElement;
}


// Create the mesh for the specified geometry
TSharedPtr<IDatasmithMeshElement> CVimToDatasmith::CreateDatasmithMesh(int32 geometryIndex)
{
	static int32_t	reorder[] = { 0, 1, 2 };
	FDatasmithMesh mesh;
	mesh.SetName(UTF8_TO_TCHAR(Utf8StringFormat("%d", geometryIndex).c_str()));

	// Collect vertex used by this geometry
	std::unordered_map<uint32_t, int32_t>	vimIndiceToDsMeshIndice;
	int32_t verticesCount = 0;
	uint32_t indicesStart = mGroupIndexOffets[geometryIndex];
	uint32_t indicesEnd = indicesStart + mGroupIndexCounts[geometryIndex];
	for (uint32_t index = indicesStart; index < indicesEnd; ++index)
	{
		uint32_t vertexIndex = mIndices[index];
		if (vimIndiceToDsMeshIndice.find(vertexIndex) == vimIndiceToDsMeshIndice.end())
			vimIndiceToDsMeshIndice[vertexIndex] = verticesCount++;
	}

	// Copy used vertex to the mesh
	mesh.SetVerticesCount(verticesCount);
	for (const auto& iter : vimIndiceToDsMeshIndice)
	{
		const cVec3& position = mPositions[iter.first];
		mesh.SetVertex(iter.second, position.x * Meter2Centimeter, -position.y * Meter2Centimeter, position.z * Meter2Centimeter);
	}

	// Collect materials used by this geometry
	std::unordered_map<uint32_t, int32_t>	vimMaterialIdToDsMeshMaterialIndice;
	int32_t materialsCount = 0;
	uint32_t vimMaterial = indicesStart / 3;

	// Copy faces used by this geometry
	int32_t facesCount = mGroupIndexCounts[geometryIndex] / 3;
	TestAssert(facesCount * 3 == mGroupIndexCounts[geometryIndex]);
	mesh.SetFacesCount(facesCount);
	uint32_t vimIndice = indicesStart;
	for (int32_t indexFace = 0; indexFace < facesCount; ++indexFace)
	{
		// Get material
		int32_t triangleMaterial = materialsCount;
		uint32_t vimMaterialId = mMaterialIds[vimMaterial++];
		auto iter = vimMaterialIdToDsMeshMaterialIndice.find(vimMaterialId);
		if (iter == vimMaterialIdToDsMeshMaterialIndice.end())
			vimMaterialIdToDsMeshMaterialIndice[vimMaterialId] = materialsCount++;
		else
			triangleMaterial = iter->second;
		
		// Get the face local vertices index.
		int32_t	triangleVertices[3];
		cVec3 normals[3];
		for (int i = 0; i < 3; ++i) {
			uint32_t indice = mIndices[vimIndice++];
			triangleVertices[reorder[i]] = vimIndiceToDsMeshIndice[indice];
			const cVec3& normal = mNormals[indice];
			mesh.SetNormal(indexFace * 3 + reorder[i], normal.x, -normal.y, normal.z);
		}
		
		mesh.SetFace(indexFace, triangleVertices[0], triangleVertices[1], triangleVertices[2], triangleMaterial);
		
		/*

		 int32_t triangleUVs[3];
		 ...
		 mesh.SetFaceUV(indexFace, UVChannel, triangleUVs[0], triangleUVs[1], triangleUVs[2]);
		 */
	}
	
	// Create a new mesh file
	FDatasmithMeshExporter MeshExporter;
	TSharedPtr<IDatasmithMeshElement> meshElement = MeshExporter.ExportToUObject(*mOutputPath, mesh.GetName(), mesh, nullptr, EDSExportLightmapUV::Never);
	if (meshElement.IsValid()) {
		meshElement->SetLabel(UTF8_TO_TCHAR(Utf8StringFormat("Geometry %d", geometryIndex).c_str()));
		for (auto& iter : vimMaterialIdToDsMeshMaterialIndice)
		{
			size_t materialIndex = mVimToDatasmithMaterialMap[iter.first];
			TestAssert(materialIndex < mMaterials.size());
			MaterialEntry& materialEntry = mMaterials[materialIndex];
			materialEntry.mCount++;
			meshElement->SetMaterial(materialEntry.mMaterialElement->GetName(), iter.second);
		}
	}

	return meshElement;
}


// In old vim files, the geometry is exported in world space, even when instanced, so we need to remove that world transform from the geometry
void CVimToDatasmith::FixOldVimFileTransforms()
{
	if (mVimScene.mVersionMajor == 0 && mVimScene.mVersionMinor == 0 && mVimScene.mVersionPatch <= 200) {
		std::vector<bool>	IsTransformed(mGroupVertexOffets.Count(), false);
		
		for (size_t nodeIndex = 0; nodeIndex < mVimScene.mNodes.size(); ++nodeIndex)
		{
			const Vim::SceneNode& node = mVimScene.mNodes[nodeIndex];
			if (node.mGeometry != kNoGeometry)
			{
				TestAssert(node.mGeometry < (int)IsTransformed.size());
				if (!IsTransformed[node.mGeometry])
				{
					IsTransformed[node.mGeometry] = true;
					TestAssert(node.mInstance == nodeIndex || node.mInstance == kNoInstance);
					
					cMat4 trans = (*(cMat4*)node.mTransform);
					cMat4 invTrans = trans.Inverse();
					
					uint32_t nextOffset = node.mGeometry < (int)mGroupVertexOffets.Count() ? mGroupVertexOffets[node.mGeometry + 1] : uint32_t(mPositions.Count());
					for (uint32_t index = mGroupVertexOffets[node.mGeometry]; index < nextOffset; ++index)
					{
						cVec3& vertex = mPositions[index];
						vertex = vertex * invTrans;
					}
				}
			}
		}
	}
}


// Initialize the converter from Vim scene
void CVimToDatasmith::ProcessGeometry()
{
	TraceF("CVimToDatasmith::ProcessGeometry\n");

	TAttributeVector<cVec4>	colorAttribute;

	for (const g3d::Attribute& attr : mVimScene.mGeometry.attributes) {
		auto descriptorString = attr.descriptor.to_string();
		if (descriptorString == g3d::descriptors::VertexColorWithAlpha)
			colorAttribute.Initialize(attr);
		else if (descriptorString == g3d::descriptors::Position)
			mPositions.Initialize(attr);
		else if (descriptorString == g3d::descriptors::Index)
			mIndices.Initialize(attr);
		else if (descriptorString == g3d::descriptors::FaceMaterialId)
			mMaterialIds.Initialize(attr);
		else if (descriptorString == g3d::descriptors::FaceGroupId)
			mObjectIds.Initialize(attr);
		else if (descriptorString == g3d::descriptors::GroupIndexOffset)
			mGroupIndexOffets.Initialize(attr);
		else if (descriptorString == g3d::descriptors::GroupVertexOffset)
			mGroupVertexOffets.Initialize(attr);
		else if (descriptorString == g3d::descriptors::VertexUv)
			mVertexUVs.Initialize(attr);
		else
			TraceF("Unprocessed attribute \"%s\"\n", descriptorString.c_str());
	}
	
	const size_t groupCount = mGroupIndexOffets.Count();
	TestAssert(groupCount == mGroupVertexOffets.Count());
	
	// Calculate group index counts
	mGroupIndexCounts.Allocate(groupCount);
	if (groupCount > 0) {
		for (uint32_t i = 0; i < groupCount - 1; ++i)
			mGroupIndexCounts[i] = mGroupIndexOffets[i + 1] - mGroupIndexOffets[i];
		mGroupIndexCounts[groupCount - 1] = uint32_t(mIndices.Count()) - mGroupIndexOffets[groupCount - 1];
	}

	FixOldVimFileTransforms();

	ComputeNormals(true);
}


// Datasmith need normals.
void CVimToDatasmith::ComputeNormals(bool inFlip)
{
	mNormals.Allocate(mPositions.Count());
	for (uint32_t i = 0; i < mIndices.Count(); i += 3)
	{
		cVec3 v0 = mPositions[mIndices[i + 0]];
		cVec3 v1 = mPositions[mIndices[i + 1]];
		cVec3 v2 = mPositions[mIndices[i + 2]];
		
		cVec3 s0 = v2 - v0;
		cVec3 s1 = v2 - v1;
		
		cVec3 normal = s1 ^ s0;
		normal.Normalise();
		if (inFlip)
			normal = -normal;

		mNormals[mIndices[i + 0]] += normal;
		mNormals[mIndices[i + 1]] += normal;
		mNormals[mIndices[i + 2]] += normal;
	}
	for (uint32_t i = 0; i < mIndices.Count(); ++i)
		mNormals[mIndices[i]].Normalise();
}


// Create all mesh actors from Vim scene nodes
void CVimToDatasmith::ProcessInstances()
{
	TraceF("CVimToDatasmith::ProcessInstances\n");
	
	for (size_t nodeIndex = 0; nodeIndex < mVimScene.mNodes.size(); nodeIndex++)
	{
		const Vim::SceneNode& node = mVimScene.mNodes[nodeIndex];
	
		// Get instance geometry definition
		int geometryIndex = kNoGeometry;
		if (node.mGeometry != kNoGeometry && (node.mInstance == kNoInstance || node.mInstance == nodeIndex))
		{
			// This node is main instance (instance class/geometry definition)
			geometryIndex = node.mGeometry;
		}
		else if (node.mInstance != kNoInstance && node.mInstance != nodeIndex && mVimScene.mNodes[node.mInstance].mGeometry != kNoGeometry)
		{
			// This node is an instance
			geometryIndex = mVimScene.mNodes[node.mInstance].mGeometry;
		}
		
		if (geometryIndex != kNoGeometry)
		{
			TSharedPtr<IDatasmithMeshElement> mesh = GetDatasmithMesh(geometryIndex);
			if (mesh.IsValid())
			{
				TSharedRef<IDatasmithMeshActorElement>	meshActor = FDatasmithSceneFactory::CreateMeshActor(UTF8_TO_TCHAR(Utf8StringFormat("%lu", nodeIndex).c_str()));
				meshActor->SetStaticMeshPathName(mesh->GetName());
				mDatasmithScene->AddActor(meshActor);

				const cMat4& nodeTrans = *reinterpret_cast<const cMat4*>(node.mTransform);
				cMat4 trans = nodeTrans.Transposed();

				FQuat	rotation(GetRotationQuat(trans));
				meshActor->SetRotation(rotation, false);

				FVector translation(GetTranslationVector(trans));
				meshActor->SetTranslation(translation, false);

				FVector scale(GetScaleVector(trans));
				meshActor->SetScale(scale, false);
			}
		}
	}
}


// Add Datasmith materials used to the scene
void CVimToDatasmith::AddUsedMaterials()
{
	for (auto& material : mMaterials)
	{
		if (material.mCount > 0)
			mDatasmithScene->AddMaterial(material.mMaterialElement);
	}
}


// Convert Vim scene to a datasmith scene
void CVimToDatasmith::CreateDatasmithScene()
{
	TraceF("CVimToDatasmith::CreateDatasmithScene\n");
	mDatasmithScene = FDatasmithSceneFactory::CreateScene(UTF8_TO_TCHAR(mVimFilePath.c_str()));
	
	mDatasmithScene->SetHost(TEXT("Vim"));
	mDatasmithScene->SetVendor(TEXT("VIMaec"));
	mDatasmithScene->SetProductName(TEXT("VimToDatasmith"));
	mDatasmithScene->SetProductVersion(UTF8_TO_TCHAR("1.0.0"));

	CreateMaterials();
	ProcessGeometry();
	PrintStats();
	ProcessInstances();
	AddUsedMaterials();
}


// Write a Datasmith scene to the Datasmith file
void CVimToDatasmith::CreateDatasmithFile()
{
	TraceF("CVimToDatasmith::CreateDatasmithFile\n");
	
	FDatasmithSceneExporter SceneExporter;
	SceneExporter.PreExport();
	SceneExporter.SetName(UTF8_TO_TCHAR(mDatasmithFileName.c_str()));
	
	SceneExporter.SetOutputPath(UTF8_TO_TCHAR(mDatasmithFolderPath.c_str()));
	
	SceneExporter.Export(mDatasmithScene.ToSharedRef());
}


// Print selected contents
void CVimToDatasmith::PrintStats()
{
	TraceF("Positions %lu, Indices=%lu, Groups=%lu, Nodes=%lu, Strings=%lu\n", mPositions.Count(), mIndices.Count(), mGroupVertexOffets.Count(), mVimScene.mNodes.size(), mVimScene.mStrings.size());

#if 0
#if 0
	TraceF("Nodes count %lu\n", mVimScene.mNodes.size());
	for (const auto& node : mVimScene.mNodes) {
		TraceF("\tNode Parent=%d, Geometry=%d, Instance=%d\n", node.mParent, node.mGeometry, node.mInstance);
		const cMat4& nodeTrans = *reinterpret_cast<const cMat4*>(node.mTransform);
		if (!IsIdentity(nodeTrans))
			TraceF("\t\tTransform={%s}\n", ToUtf8(nodeTrans));
	}
#endif
#if 0
	TraceF("String count %lu\n", mVimScene.mStrings.size());
	for (size_t index = 0; index < mVimScene.mStrings.size(); ++index)
		TraceF("\t%lu \"%s\"\n", index, mVimScene.mStrings[index]);
#endif

	TraceF("Entities count= %ld\n", mVimScene.mEntityTables.size());
	for (const auto& entity : mVimScene.mEntityTables)
		TraceF("\t%s\n", entity.first.c_str());
	
	TraceF("Assets count= %ld\n", mVimScene.mAssetsBFast.buffers.size());
	for (const auto& asset : mVimScene.mAssetsBFast.buffers)
		TraceF("\t%s\n", asset.name.c_str());

	mPositions.Print("Positions");
	mIndices.Print("Indices");
	mMaterialIds.Print("MaterialIds");
	mObjectIds.Print("ObjectIdsX");
	mGroupIndexOffets.Print("GroupIndexOffets");
	mGroupVertexOffets.Print("GroupVertexOffets");

	mGroupIndexCounts.Print("GroupIndexCounts");
#endif
}


EndVim2DatasmithNameSpace
