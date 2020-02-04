// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MeshDescriptionBuilder.h"
#include "StaticMeshAttributes.h"
#include "VectorTypes.h"
#include "BoxTypes.h"

#include "DynamicMesh3.h"
#include "DynamicMeshAttributeSet.h"

#include "MeshNormals.h"

namespace ExtendedMeshAttribute
{
	const FName PolyTriGroups("PolyTriGroups");
}



void FMeshDescriptionBuilder::SetMeshDescription(FMeshDescription* Description)
{
	this->MeshDescription = Description;
	this->VertexPositions = 
		MeshDescription->VertexAttributes().GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);
	this->InstanceUVs =
		MeshDescription->VertexInstanceAttributes().GetAttributesRef<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);
	this->InstanceNormals =
		MeshDescription->VertexInstanceAttributes().GetAttributesRef<FVector>(MeshAttribute::VertexInstance::Normal);
	this->InstanceColors =
		MeshDescription->VertexInstanceAttributes().GetAttributesRef<FVector4>(MeshAttribute::VertexInstance::Color);
}


void FMeshDescriptionBuilder::EnablePolyGroups()
{
	PolyGroups =
		MeshDescription->PolygonAttributes().GetAttributesRef<int>(ExtendedMeshAttribute::PolyTriGroups);
	if (PolyGroups.IsValid() == false)
	{
		MeshDescription->PolygonAttributes().RegisterAttribute<int>(
			ExtendedMeshAttribute::PolyTriGroups, 1, 0, EMeshAttributeFlags::AutoGenerated);
		PolyGroups =
			MeshDescription->PolygonAttributes().GetAttributesRef<int>(ExtendedMeshAttribute::PolyTriGroups);
		check(PolyGroups.IsValid());
	}
}


FVertexID FMeshDescriptionBuilder::AppendVertex(const FVector& Position)
{
	FVertexID VertexID = MeshDescription->CreateVertex();
	VertexPositions.Set(VertexID, Position);
	return VertexID;
}


FPolygonGroupID FMeshDescriptionBuilder::AppendPolygonGroup()
{
	return MeshDescription->CreatePolygonGroup();
}



FPolygonID FMeshDescriptionBuilder::AppendTriangle(const FVertexID& Vertex0, const FVertexID& Vertex1, const FVertexID& Vertex2, const FPolygonGroupID& PolygonGroup)
{
	TempBuffer.SetNum(3, false);
	TempBuffer[0] = Vertex0;
	TempBuffer[1] = Vertex1;
	TempBuffer[2] = Vertex2;
	return AppendPolygon(TempBuffer, PolygonGroup);
}


FVertexInstanceID FMeshDescriptionBuilder::AppendInstance(const FVertexID& VertexID)
{
	return MeshDescription->CreateVertexInstance(VertexID);
}



void FMeshDescriptionBuilder::SetPosition(const FVertexID& VertexID, const FVector& NewPosition)
{
	VertexPositions.Set(VertexID, 0, NewPosition);
}

FVector FMeshDescriptionBuilder::GetPosition(const FVertexID& VertexID)
{
	return VertexPositions.Get(VertexID, 0);
}

FVector FMeshDescriptionBuilder::GetPosition(const FVertexInstanceID& InstanceID)
{
	return VertexPositions.Get(MeshDescription->GetVertexInstanceVertex(InstanceID), 0);
}


void FMeshDescriptionBuilder::SetInstance(const FVertexInstanceID& InstanceID, const FVector2D& InstanceUV, const FVector& InstanceNormal)
{
	if (InstanceUVs.IsValid())
	{
		InstanceUVs.Set(InstanceID, InstanceUV); 
	}
	if (InstanceNormals.IsValid())
	{
		InstanceNormals.Set(InstanceID, InstanceNormal);
	}
}


void FMeshDescriptionBuilder::SetInstanceNormal(const FVertexInstanceID& InstanceID, const FVector& Normal)
{
	if (InstanceNormals.IsValid())
	{
		InstanceNormals.Set(InstanceID, Normal);
	}
}


void FMeshDescriptionBuilder::SetInstanceUV(const FVertexInstanceID& InstanceID, const FVector2D& InstanceUV, int32 UVLayerIndex)
{
	if (InstanceUVs.IsValid() && ensure(UVLayerIndex < InstanceUVs.GetNumIndices()))
	{
		InstanceUVs.Set(InstanceID, UVLayerIndex, InstanceUV); 
	}
}


void FMeshDescriptionBuilder::SetNumUVLayers(int32 NumUVLayers)
{
	if (ensure(InstanceUVs.IsValid()))
	{
		InstanceUVs.SetNumIndices(NumUVLayers);
	}
}


void FMeshDescriptionBuilder::SetInstanceColor(const FVertexInstanceID& InstanceID, const FVector4& Color)
{
	if (InstanceColors.IsValid())
	{
		InstanceColors.Set(InstanceID, Color);
	}
}


FPolygonID FMeshDescriptionBuilder::AppendTriangle(const FVertexID* Triangle, const FPolygonGroupID& PolygonGroup, 
	const FVector2D * VertexUVs, const FVector* VertexNormals)
{
	TempBuffer.SetNum(3, false);
	TempBuffer[0] = Triangle[0];
	TempBuffer[1] = Triangle[1];
	TempBuffer[2] = Triangle[2];

	const TArray<FVector2D> * UseUVs = nullptr;
	if (VertexUVs != nullptr) 
	{
		UVBuffer.SetNum(3, false);
		UVBuffer[0] = VertexUVs[0];
		UVBuffer[1] = VertexUVs[1];
		UVBuffer[2] = VertexUVs[2];
		UseUVs = &UVBuffer;
	}

	const TArray<FVector> * UseNormals = nullptr;
	if (VertexNormals != nullptr)
	{
		NormalBuffer.SetNum(3, false);
		NormalBuffer[0] = VertexNormals[0];
		NormalBuffer[1] = VertexNormals[1];
		NormalBuffer[2] = VertexNormals[2];
		UseNormals = &NormalBuffer;
	}

	return AppendPolygon(TempBuffer, PolygonGroup, UseUVs, UseNormals);
}


FPolygonID FMeshDescriptionBuilder::AppendPolygon(const TArray<FVertexID>& Vertices, const FPolygonGroupID& PolygonGroup, 
	const TArray<FVector2D> * VertexUVs, const TArray<FVector>* VertexNormals)
{
	int NumVertices = Vertices.Num();
	TArray<FVertexInstanceID> Polygon; 
	Polygon.Reserve(NumVertices);
	for (int j = 0; j < NumVertices; ++j) 
	{
		FVertexInstanceID VertexInstance = MeshDescription->CreateVertexInstance(Vertices[j]);
		Polygon.Add(VertexInstance);

		if (VertexUVs != nullptr)
		{
			InstanceUVs.Set(VertexInstance, (*VertexUVs)[j]);
		}
		if (VertexNormals != nullptr)
		{
			InstanceNormals.Set(VertexInstance, (*VertexNormals)[j]);
		}

	}

	const FPolygonID NewPolygonID = MeshDescription->CreatePolygon(PolygonGroup, Polygon);

	return NewPolygonID;
}




FPolygonID FMeshDescriptionBuilder::AppendTriangle(const FVertexInstanceID& Instance0, const FVertexInstanceID& Instance1, const FVertexInstanceID& Instance2, const FPolygonGroupID& PolygonGroup)
{
	TArray<FVertexInstanceID> Polygon;
	Polygon.Add(Instance0);
	Polygon.Add(Instance1);
	Polygon.Add(Instance2);

	const FPolygonID NewPolygonID = MeshDescription->CreatePolygon(PolygonGroup, Polygon);

	return NewPolygonID;
}





void FMeshDescriptionBuilder::SetPolyGroupID(const FPolygonID& PolygonID, int GroupID)
{
	PolyGroups.Set(PolygonID, 0, GroupID);
}









void FMeshDescriptionBuilder::Translate(const FVector& Translation)
{
	for (FVertexID VertexID : MeshDescription->Vertices().GetElementIDs())
	{
		FVector Position = VertexPositions.Get(VertexID);
		Position += Translation;
		VertexPositions.Set(VertexID, Position);
	}
}




void FMeshDescriptionBuilder::SetAllEdgesHardness(bool bHard)
{
	TEdgeAttributesRef<bool> EdgeHardness =
		MeshDescription->EdgeAttributes().GetAttributesRef<bool>(MeshAttribute::Edge::IsHard);
	for (FEdgeID EdgeID : MeshDescription->Edges().GetElementIDs())
	{
		EdgeHardness.Set(EdgeID, 0, bHard);
	}
}




FBox FMeshDescriptionBuilder::ComputeBoundingBox() const
{
	FAxisAlignedBox3f bounds = FAxisAlignedBox3f::Empty();
	for ( FVertexID VertexID : MeshDescription->Vertices().GetElementIDs() )
	{
		bounds.Contain(VertexPositions.Get(VertexID));
	}
	return bounds;
}