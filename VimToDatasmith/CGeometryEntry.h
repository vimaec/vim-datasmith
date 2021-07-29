// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CVimToDatasmith.h"

namespace Vim2Ds {

// GeometryEntry is a definition and an array of instances
class CVimToDatasmith::CGeometryEntry : CTaskMgr::ITask {
  public:
    // Constructor
    CGeometryEntry(CVimToDatasmith* inVimToDatasmith, GeometryIndex inGeometry, NodeIndex inDefinition);

    // Add an instance.
    void AddInstance(NodeIndex inInstance);

    // Create Datasmith actors
    void CreateActors();

  private:
    // Process the node's geometry (create datasmith mesh)
    void Run();

    void ConvertGeometryToDatasmithMesh(FDatasmithMesh* outMesh, MapVimMaterialIdToDsMeshMaterialIndice* outVimMaterialIdToDsMeshMaterialIndice);

    void AddActor(const TSharedRef<IDatasmithMeshActorElement>& inActor, NodeIndex inInstance);

    // Create an actor for the specified node
    void CreateActor(NodeIndex inInstance);

    // Create an efficient actor for the specified instance
    void CreateHierarchicalInstancesActor();

    // Create the actor name based on it's content (take care of duplicates)
    FString HashToName(Datasmith::FDatasmithHash& hasher, NodeIndex inInstance) const;

    CVimToDatasmith* const mVimToDatasmith; // The converter
    CMeshElement* mMeshElement = nullptr; // The mesh element that is geometry and affected material.
    GeometryIndex mGeometry = GeometryIndex::kNoGeometry;
    NodeIndex mDefinition = NodeIndex::kNoNode; // First instance is considered as the definition
    std::unique_ptr<std::vector<NodeIndex>> mInstances; // All other instances (exclude definition one)
};

} // namespace Vim2Ds
