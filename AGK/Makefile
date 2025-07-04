ARCH := $(shell getconf LONG_BIT)

OUT32 = platform/linux/Lib/Release32
OUT64 = platform/linux/Lib/Release64
OUT = $(OUT$(ARCH))/libAGKLinux.a
CC = g++
ODIR = $(OUT$(ARCH))/obj
INC = -Icommon/include -Icommon -Igiflib -Izxing -Ilibpng -Ilibjpeg -Izlib -Iassimp -Ibullet -Ibullet/BulletCollision/CollisionShapes -Ilibogg/include -Ifreetype/include -Icurl -Icurl/include 

CFLAGS = -O2
CPPFLAGS = -O2 -std=c++11

_OBJS = AGKMotionState.o \
        AGKToBullet.o \
        BaseItem.o \
        CollisionShapes.o \
        ContactReport.o \
        DynamicsWorld.o \
        GLDebugDrawer.o \
        Joint.o \
        Joints.o \
        Ray.o \
        RigidBodies.o \
        RigidBody.o \
        StaticPlane.o \
        CharacterController.o \
        Ragdoll.o \
        RagdollBone.o \
        Vector.o \
        3DMath.o \
        AGKLights.o \
        AGKShader.o \
        cCamera.o \
        cFile.o \
        cForce.o \
        cHashedList.o \
        cImage.o \
        cNode.o \
        cObject3D.o \
        cObjectMgr.o \
        cSound.o \
        cSprite.o \
        cSpriteMgr.o \
        cText.o \
        cTextEntry.o \
        AGKFont.o \
        cTouch.o \
        DebugDraw.o \
        FrameBuffer.o \
        Joystick.o \
        JSONElement.o \
        Mouse.o \
        Network.o \
        Particles.o \
        Physics.o \
        sha1.o \
        sha2.o \
        AGO.o \
        cMesh.o \
        cMaterial.o \
        Skeleton2D.o \
        Skeleton3D.o \
        Tween.o \
        uString.o \
        Wrapper.o\
        ZipFile.o \
        AGKSkyBox.o \
        3DParticles.o \
		SimplexNoise.o \
        Collision/Box.o \
        Collision/CollisionBox.o \
        Collision/CollisionPolygon.o \
        Collision/CollisionResults.o \
        Collision/CollisionSphere.o \
        Collision/CollisionTree.o \
        Collision/Face.o \
        Collision/TreeNode.o \
        LinuxNetwork.o \
        LinuxCore.o \
        OpenGL2.o \
        Box2D/Dynamics/b2WorldCallbacks.o \
        Box2D/Dynamics/b2World.o \
        Box2D/Dynamics/b2Island.o \
        Box2D/Dynamics/b2Fixture.o \
        Box2D/Dynamics/b2ContactManager.o \
        Box2D/Dynamics/b2Body.o \
        Box2D/Collision/b2TimeOfImpact.o \
        Box2D/Collision/b2DynamicTree.o \
        Box2D/Collision/b2Distance.o \
        Box2D/Collision/b2Collision.o \
        Box2D/Collision/b2CollidePolygon.o \
        Box2D/Collision/b2CollideEdge.o \
        Box2D/Collision/b2CollideCircle.o \
        Box2D/Collision/b2BroadPhase.o \
        Box2D/Collision/Shapes/b2PolygonShape.o \
        Box2D/Collision/Shapes/b2EdgeShape.o \
        Box2D/Collision/Shapes/b2CircleShape.o \
        Box2D/Collision/Shapes/b2ChainShape.o \
        Box2D/Common/b2Timer.o \
        Box2D/Common/b2StackAllocator.o \
        Box2D/Common/b2Settings.o \
        Box2D/Common/b2Math.o \
        Box2D/Common/b2Draw.o \
        Box2D/Common/b2BlockAllocator.o \
        Box2D/Dynamics/Contacts/b2PolygonContact.o \
        Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.o \
        Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.o \
        Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.o \
        Box2D/Dynamics/Contacts/b2ContactSolver.o \
        Box2D/Dynamics/Contacts/b2Contact.o \
        Box2D/Dynamics/Contacts/b2CircleContact.o \
        Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.o \
        Box2D/Dynamics/Contacts/b2ChainAndCircleContact.o \
        Box2D/Dynamics/Joints/b2WheelJoint.o \
        Box2D/Dynamics/Joints/b2WeldJoint.o \
        Box2D/Dynamics/Joints/b2RopeJoint.o \
        Box2D/Dynamics/Joints/b2RevoluteJoint.o \
        Box2D/Dynamics/Joints/b2PulleyJoint.o \
        Box2D/Dynamics/Joints/b2PrismaticJoint.o \
        Box2D/Dynamics/Joints/b2MouseJoint.o \
        Box2D/Dynamics/Joints/b2Joint.o \
        Box2D/Dynamics/Joints/b2GearJoint.o \
        Box2D/Dynamics/Joints/b2FrictionJoint.o \
        Box2D/Dynamics/Joints/b2DistanceJoint.o \
        Box2D/Dynamics/Joints/b2MotorJoint.o \
        Box2D/Rope/b2Rope.o \
        zlib/adler32.o \
        zlib/compress.o \
        zlib/crc32.o \
        zlib/deflate.o \
        zlib/gzclose.o \
        zlib/gzlib.o \
        zlib/gzread.o \
        zlib/gzwrite.o \
        zlib/infback.o \
        zlib/inffast.o \
        zlib/inflate.o \
        zlib/inftrees.o \
        zlib/ioapi.o \
        zlib/trees.o \
        zlib/uncompr.o \
        zlib/unzip.o \
        zlib/zip.o \
        zlib/zutil.o \
        zxing/BarcodeFormat.o \
        zxing/Binarizer.o \
        zxing/BinaryBitmap.o \
        zxing/DecodeHints.o \
        zxing/Exception.o \
        zxing/FormatException.o \
        zxing/LuminanceSource.o \
        zxing/MultiFormatReader.o \
        zxing/NotFoundException.o \
        zxing/QREncode.o \
        zxing/Reader.o \
        zxing/ReaderException.o \
        zxing/Result.o \
        zxing/ResultPoint.o \
        zxing/ResultPointCallback.o \
        zxing/common/Array.o \
        zxing/common/BitArray.o \
        zxing/common/BitMatrix.o \
        zxing/common/BitSource.o \
        zxing/common/CharacterSetECI.o \
        zxing/common/Counted.o \
        zxing/common/DecoderResult.o \
        zxing/common/DetectorResult.o \
        zxing/common/ECI.o \
        zxing/common/EdgeDetector.o \
        zxing/common/GlobalHistogramBinarizer.o \
        zxing/common/GreyscaleLuminanceSource.o \
        zxing/common/GreyscaleRotatedLuminanceSource.o \
        zxing/common/GridSampler.o \
        zxing/common/HybridBinarizer.o \
        zxing/common/IllegalArgumentException.o \
        zxing/common/PerspectiveTransform.o \
        zxing/common/Str.o \
        zxing/common/StringUtils.o \
        zxing/common/detector/MonochromeRectangleDetector.o \
        zxing/common/detector/WhiteRectangleDetector.o \
        zxing/common/reedsolomon/GF256.o \
        zxing/common/reedsolomon/GF256Poly.o \
        zxing/common/reedsolomon/ReedSolomonDecoder.o \
        zxing/common/reedsolomon/ReedSolomonException.o \
        zxing/multi/ByQuadrantReader.o \
        zxing/multi/GenericMultipleBarcodeReader.o \
        zxing/multi/MultipleBarcodeReader.o \
        zxing/multi/qrcode/QRCodeMultiReader.o \
        zxing/multi/qrcode/detector/MultiDetector.o \
        zxing/multi/qrcode/detector/MultiFinderPatternFinder.o \
        zxing/oned/Code39Reader.o \
        zxing/oned/Code128Reader.o \
        zxing/oned/EAN8Reader.o \
        zxing/oned/EAN13Reader.o \
        zxing/oned/ITFReader.o \
        zxing/oned/MultiFormatOneDReader.o \
        zxing/oned/MultiFormatUPCEANReader.o \
        zxing/oned/OneDReader.o \
        zxing/oned/OneDResultPoint.o \
        zxing/oned/UPCAReader.o \
        zxing/oned/UPCEANReader.o \
        zxing/oned/UPCEReader.o \
        zxing/qrcode/ErrorCorrectionLevel.o \
        zxing/qrcode/FormatInformation.o \
        zxing/qrcode/QRCodeReader.o \
        zxing/qrcode/Version.o \
        zxing/qrcode/decoder/BitMatrixParser.o \
        zxing/qrcode/decoder/DataBlock.o \
        zxing/qrcode/decoder/DataMask.o \
        zxing/qrcode/decoder/DecodedBitStreamParser.o \
        zxing/qrcode/decoder/Decoder.o \
        zxing/qrcode/decoder/Mode.o \
        zxing/qrcode/detector/AlignmentPattern.o \
        zxing/qrcode/detector/AlignmentPatternFinder.o \
        zxing/qrcode/detector/Detector.o \
        zxing/qrcode/detector/FinderPattern.o \
        zxing/qrcode/detector/FinderPatternFinder.o \
        zxing/qrcode/detector/FinderPatternInfo.o \
        zxing/qrcode/detector/QREdgeDetector.o \
        zxing/datamatrix/DataMatrixReader.o \
        zxing/datamatrix/Version2.o \
        zxing/datamatrix/decoder/BitMatrixParser2.o \
        zxing/datamatrix/decoder/DataBlock2.o \
        zxing/datamatrix/decoder/DecodedBitStreamParser2.o \
        zxing/datamatrix/decoder/Decoder2.o \
        zxing/datamatrix/detector/CornerPoint.o \
        zxing/datamatrix/detector/Detector2.o \
        zxing/datamatrix/detector/DetectorException.o \
        zxing/datamatrix/detector/MonochromeRectangleDetector2.o \
        libpng/png.o \
        libpng/pngerror.o \
        libpng/pngget.o \
        libpng/pngmem.o \
        libpng/pngpread.o \
        libpng/pngread.o \
        libpng/pngrio.o \
        libpng/pngrtran.o \
        libpng/pngrutil.o \
        libpng/pngset.o \
        libpng/pngtrans.o \
        libpng/pngwio.o \
        libpng/pngwrite.o \
        libpng/pngwtran.o \
        libpng/pngwutil.o \
        libjpeg/jdatasrc.o \
        libjpeg/jdapimin.o \
        libjpeg/jdmarker.o \
        libjpeg/jerror.o \
        libjpeg/jdinput.o \
        libjpeg/jdapistd.o \
        libjpeg/jcomapi.o \
        libjpeg/jmemmgr.o \
        libjpeg/jutils.o \
        libjpeg/jdmaster.o \
        libjpeg/jmemansi.o \
        libjpeg/jquant1.o \
        libjpeg/jquant2.o \
        libjpeg/jddctmgr.o \
        libjpeg/jdarith.o \
        libjpeg/jdcoefct.o \
        libjpeg/jdcolor.o \
        libjpeg/jdsample.o \
        libjpeg/jdpostct.o \
        libjpeg/jdhuff.o \
        libjpeg/jdmerge.o \
        libjpeg/jdmainct.o \
        libjpeg/jidctint.o \
        libjpeg/jaricom.o \
        libjpeg/jidctfst.o \
        libjpeg/jidctflt.o \
        libjpeg/jcapimin.o \
        libjpeg/jdatadst.o \
        libjpeg/jcparam.o \
        libjpeg/jcapistd.o \
        libjpeg/jcmarker.o \
        libjpeg/jcinit.o \
        libjpeg/jcmaster.o \
        libjpeg/jcmaster.o \
        libjpeg/jcdctmgr.o \
        libjpeg/jcarith.o \
        libjpeg/jccoefct.o \
        libjpeg/jcmainct.o \
        libjpeg/jchuff.o \
        libjpeg/jccolor.o \
        libjpeg/jcsample.o \
        libjpeg/jcprepct.o \
        libjpeg/jfdctint.o \
        libjpeg/jfdctfst.o \
        libjpeg/jfdctflt.o \
        giflib/dev2gif.o \
        giflib/dgif_lib.o \
        giflib/egif_lib.o \
        giflib/getarg.o \
        giflib/gif_err.o \
        giflib/gif_font.o \
        giflib/gif_hash.o \
        giflib/gifalloc.o \
        giflib/qprintf.o \
        giflib/quantize.o \
        assimp/3DSConverter.o \
        assimp/3DSLoader.o \
        assimp/ACLoader.o \
        assimp/ASELoader.o \
        assimp/ASEParser.o \
        assimp/Assimp.o \
        assimp/AssimpCExport.o \
        assimp/AssimpPCH.o \
        assimp/B3DImporter.o \
        assimp/BaseImporter.o \
        assimp/BaseProcess.o \
        assimp/BlenderDNA.o \
        assimp/BlenderLoader.o \
        assimp/BlenderModifier.o \
        assimp/BlenderScene.o \
        assimp/BVHLoader.o \
        assimp/CalcTangentsProcess.o \
        assimp/COBLoader.o \
        assimp/ColladaExporter.o \
        assimp/ColladaLoader.o \
        assimp/ColladaParser.o \
        assimp/ComputeUVMappingProcess.o \
        assimp/ConvertToLHProcess.o \
        assimp/CSMLoader.o \
        assimp/DeboneProcess.o \
        assimp/DefaultIOStream.o \
        assimp/DefaultIOSystem.o \
        assimp/DefaultLogger.o \
        assimp/DXFLoader.o \
        assimp/Exporter.o \
        assimp/FindDegenerates.o \
        assimp/FindInstancesProcess.o \
        assimp/FindInvalidDataProcess.o \
        assimp/FixNormalsStep.o \
        assimp/GenFaceNormalsProcess.o \
        assimp/GenVertexNormalsProcess.o \
        assimp/HMPLoader.o \
        assimp/IFCCurve.o \
        assimp/IFCGeometry.o \
        assimp/IFCLoader.o \
        assimp/IFCMaterial.o \
        assimp/IFCProfile.o \
        assimp/IFCReaderGen.o \
        assimp/IFCUtil.o \
        assimp/Importer.o \
        assimp/ImporterRegistry.o \
        assimp/ImproveCacheLocality.o \
        assimp/IRRLoader.o \
        assimp/IRRMeshLoader.o \
        assimp/IRRShared.o \
        assimp/JoinVerticesProcess.o \
        assimp/LimitBoneWeightsProcess.o \
        assimp/LWOAnimation.o \
        assimp/LWOBLoader.o \
        assimp/LWOLoader.o \
        assimp/LWOMaterial.o \
        assimp/LWSLoader.o \
        assimp/MakeVerboseFormat.o \
        assimp/MaterialSystem.o \
        assimp/MD2Loader.o \
        assimp/MD3Loader.o \
        assimp/MD5Loader.o \
        assimp/MD5Parser.o \
        assimp/MDCLoader.o \
        assimp/MDLLoader.o \
        assimp/MDLMaterialLoader.o \
        assimp/MS3DLoader.o \
        assimp/NDOLoader.o \
        assimp/NFFLoader.o \
        assimp/ObjExporter.o \
        assimp/ObjFileImporter.o \
        assimp/ObjFileMtlImporter.o \
        assimp/ObjFileParser.o \
        assimp/OFFLoader.o \
        assimp/OgreImporter.o \
        assimp/OgreMaterial.o \
        assimp/OptimizeGraph.o \
        assimp/OptimizeMeshes.o \
        assimp/PlyExporter.o \
        assimp/PlyLoader.o \
        assimp/PlyParser.o \
        assimp/PostStepRegistry.o \
        assimp/PretransformVertices.o \
        assimp/ProcessHelper.o \
        assimp/Q3BSPFileImporter.o \
        assimp/Q3BSPFileParser.o \
        assimp/Q3BSPZipArchive.o \
        assimp/Q3DLoader.o \
        assimp/RawLoader.o \
        assimp/RemoveComments.o \
        assimp/RemoveRedundantMaterials.o \
        assimp/RemoveVCProcess.o \
        assimp/SceneCombiner.o \
        assimp/ScenePreprocessor.o \
        assimp/SGSpatialSort.o \
        assimp/SkeletonMeshBuilder.o \
        assimp/SMDLoader.o \
        assimp/SortByPTypeProcess.o \
        assimp/SpatialSort.o \
        assimp/SplitByBoneCountProcess.o \
        assimp/SplitLargeMeshes.o \
        assimp/StandardShapes.o \
        assimp/STEPFileReader.o \
        assimp/STLExporter.o \
        assimp/STLLoader.o \
        assimp/Subdivision.o \
        assimp/TargetAnimation.o \
        assimp/TerragenLoader.o \
        assimp/TextureTransform.o \
        assimp/TriangulateProcess.o \
        assimp/UnrealLoader.o \
        assimp/ValidateDataStructure.o \
        assimp/VertexTriangleAdjacency.o \
        assimp/XFileImporter.o \
        assimp/XFileParser.o \
        assimp/XGLLoader.o \
        assimp/include/ConvertUTF/ConvertUTF.o \
        assimp/include/irrXML/irrXML.o \
        assimp/include/poly2tri/common/shapes.o \
        assimp/include/poly2tri/sweep/advancing_front.o \
        assimp/include/poly2tri/sweep/cdt.o \
        assimp/include/poly2tri/sweep/sweep.o \
        assimp/include/poly2tri/sweep/sweep_context.o \
        assimp/Bitmap.o \
        assimp/BlenderBMesh.o \
        assimp/BlenderTessellator.o \
        assimp/FBXAnimation.o \
        assimp/FBXBinaryTokenizer.o \
        assimp/FBXConverter.o \
        assimp/FBXDeformer.o \
        assimp/FBXDocument.o \
        assimp/FBXDocumentUtil.o \
        assimp/FBXImporter.o \
        assimp/FBXMaterial.o \
        assimp/FBXMeshGeometry.o \
        assimp/FBXModel.o \
        assimp/FBXNodeAttribute.o \
        assimp/FBXParser.o \
        assimp/FBXProperties.o \
        assimp/FBXTokenizer.o \
        assimp/FBXUtil.o \
        assimp/IFCBoolean.o \
        assimp/IFCOpenings.o \
        assimp/OgreBinarySerializer.o \
        assimp/OgreStructs.o \
        assimp/OgreXmlSerializer.o \
        assimp/STEPFileEncoding.o \
        bullet/BulletCollision/BroadphaseCollision/btAxisSweep3.o \
        bullet/BulletCollision/BroadphaseCollision/btBroadphaseProxy.o \
        bullet/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.o \
        bullet/BulletCollision/BroadphaseCollision/btDbvt.o \
        bullet/BulletCollision/BroadphaseCollision/btDbvtBroadphase.o \
        bullet/BulletCollision/BroadphaseCollision/btDispatcher.o \
        bullet/BulletCollision/BroadphaseCollision/btMultiSapBroadphase.o \
        bullet/BulletCollision/BroadphaseCollision/btOverlappingPairCache.o \
        bullet/BulletCollision/BroadphaseCollision/btQuantizedBvh.o \
        bullet/BulletCollision/BroadphaseCollision/btSimpleBroadphase.o \
        bullet/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btBoxBoxDetector.o \
        bullet/BulletCollision/CollisionDispatch/btCollisionDispatcher.o \
        bullet/BulletCollision/CollisionDispatch/btCollisionObject.o \
        bullet/BulletCollision/CollisionDispatch/btCollisionWorld.o \
        bullet/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.o \
        bullet/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btGhostObject.o \
        bullet/BulletCollision/CollisionDispatch/btHashedSimplePairCache.o \
        bullet/BulletCollision/CollisionDispatch/btInternalEdgeUtility.o \
        bullet/BulletCollision/CollisionDispatch/btManifoldResult.o \
        bullet/BulletCollision/CollisionDispatch/btSimulationIslandManager.o \
        bullet/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.o \
        bullet/BulletCollision/CollisionDispatch/btUnionFind.o \
        bullet/BulletCollision/CollisionDispatch/SphereTriangleDetector.o \
        bullet/BulletCollision/CollisionShapes/btBox2dShape.o \
        bullet/BulletCollision/CollisionShapes/btBoxShape.o \
        bullet/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.o \
        bullet/BulletCollision/CollisionShapes/btCapsuleShape.o \
        bullet/BulletCollision/CollisionShapes/btCollisionShape.o \
        bullet/BulletCollision/CollisionShapes/btCompoundShape.o \
        bullet/BulletCollision/CollisionShapes/btConcaveShape.o \
        bullet/BulletCollision/CollisionShapes/btConeShape.o \
        bullet/BulletCollision/CollisionShapes/btConvex2dShape.o \
        bullet/BulletCollision/CollisionShapes/btConvexHullShape.o \
        bullet/BulletCollision/CollisionShapes/btConvexInternalShape.o \
        bullet/BulletCollision/CollisionShapes/btConvexPointCloudShape.o \
        bullet/BulletCollision/CollisionShapes/btConvexPolyhedron.o \
        bullet/BulletCollision/CollisionShapes/btConvexShape.o \
        bullet/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.o \
        bullet/BulletCollision/CollisionShapes/btCylinderShape.o \
        bullet/BulletCollision/CollisionShapes/btEmptyShape.o \
        bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.o \
        bullet/BulletCollision/CollisionShapes/btMinkowskiSumShape.o \
        bullet/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.o \
        bullet/BulletCollision/CollisionShapes/btMultiSphereShape.o \
        bullet/BulletCollision/CollisionShapes/btOptimizedBvh.o \
        bullet/BulletCollision/CollisionShapes/btPolyhedralConvexShape.o \
        bullet/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.o \
        bullet/BulletCollision/CollisionShapes/btShapeHull.o \
        bullet/BulletCollision/CollisionShapes/btSphereShape.o \
        bullet/BulletCollision/CollisionShapes/btStaticPlaneShape.o \
        bullet/BulletCollision/CollisionShapes/btStridingMeshInterface.o \
        bullet/BulletCollision/CollisionShapes/btTetrahedronShape.o \
        bullet/BulletCollision/CollisionShapes/btTriangleBuffer.o \
        bullet/BulletCollision/CollisionShapes/btTriangleCallback.o \
        bullet/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.o \
        bullet/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.o \
        bullet/BulletCollision/CollisionShapes/btTriangleMesh.o \
        bullet/BulletCollision/CollisionShapes/btTriangleMeshShape.o \
        bullet/BulletCollision/CollisionShapes/btUniformScalingShape.o \
        bullet/BulletCollision/Gimpact/btContactProcessing.o \
        bullet/BulletCollision/Gimpact/btGenericPoolAllocator.o \
        bullet/BulletCollision/Gimpact/btGImpactBvh.o \
        bullet/BulletCollision/Gimpact/btGImpactCollisionAlgorithm.o \
        bullet/BulletCollision/Gimpact/btGImpactQuantizedBvh.o \
        bullet/BulletCollision/Gimpact/btGImpactShape.o \
        bullet/BulletCollision/Gimpact/btTriangleShapeEx.o \
        bullet/BulletCollision/Gimpact/gim_box_set.o \
        bullet/BulletCollision/Gimpact/gim_contact.o \
        bullet/BulletCollision/Gimpact/gim_memory.o \
        bullet/BulletCollision/Gimpact/gim_tri_collision.o \
        bullet/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.o \
        bullet/BulletCollision/NarrowPhaseCollision/btConvexCast.o \
        bullet/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.o \
        bullet/BulletCollision/NarrowPhaseCollision/btGjkEpa2.o \
        bullet/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.o \
        bullet/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.o \
        bullet/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.o \
        bullet/BulletCollision/NarrowPhaseCollision/btPersistentManifold.o \
        bullet/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.o \
        bullet/BulletCollision/NarrowPhaseCollision/btRaycastCallback.o \
        bullet/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.o \
        bullet/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.o \
        bullet/BulletDynamics/Character/btKinematicCharacterController.o \
        bullet/BulletDynamics/ConstraintSolver/btConeTwistConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btContactConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btFixedConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btGearConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btHinge2Constraint.o \
        bullet/BulletDynamics/ConstraintSolver/btHingeConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btPoint2PointConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.o \
        bullet/BulletDynamics/ConstraintSolver/btSliderConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btTypedConstraint.o \
        bullet/BulletDynamics/ConstraintSolver/btUniversalConstraint.o \
        bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.o \
        bullet/BulletDynamics/Dynamics/btRigidBody.o \
        bullet/BulletDynamics/Dynamics/btSimpleDynamicsWorld.o \
        bullet/BulletDynamics/Dynamics/Bullet-C-API.o \
        bullet/BulletDynamics/Featherstone/btMultiBody.o \
        bullet/BulletDynamics/Featherstone/btMultiBodyConstraint.o \
        bullet/BulletDynamics/Featherstone/btMultiBodyConstraintSolver.o \
        bullet/BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.o \
        bullet/BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.o \
        bullet/BulletDynamics/Featherstone/btMultiBodyJointMotor.o \
        bullet/BulletDynamics/Featherstone/btMultiBodyPoint2Point.o \
        bullet/BulletDynamics/MLCPSolvers/btDantzigLCP.o \
        bullet/BulletDynamics/MLCPSolvers/btMLCPSolver.o \
        bullet/BulletDynamics/Vehicle/btRaycastVehicle.o \
        bullet/BulletDynamics/Vehicle/btWheelInfo.o \
        bullet/BulletFileLoader/bChunk.o \
        bullet/BulletFileLoader/bDNA.o \
        bullet/BulletFileLoader/bFile.o \
        bullet/BulletFileLoader/btBulletFile.o \
        bullet/BulletWorldImporter/btBulletWorldImporter.o \
        bullet/BulletWorldImporter/btWorldImporter.o \
        bullet/LinearMath/btAlignedAllocator.o \
        bullet/LinearMath/btConvexHull.o \
        bullet/LinearMath/btConvexHullComputer.o \
        bullet/LinearMath/btGeometryUtil.o \
        bullet/LinearMath/btPolarDecomposition.o \
        bullet/LinearMath/btQuickprof.o \
        bullet/LinearMath/btSerializer.o \
        bullet/LinearMath/btVector3.o \
        libogg/src/mdct.o \
        libogg/src/psy.o \
        libogg/src/registry.o \
        libogg/src/res0.o \
        libogg/src/sharedbook.o \
        libogg/src/smallft.o \
        libogg/src/synthesis.o \
        libogg/src/vorbisenc.o \
        libogg/src/vorbisfile.o \
        libogg/src/window.o \
        libogg/src/analysis.o \
        libogg/src/bitrate.o \
        libogg/src/bitwise.o \
        libogg/src/block.o \
        libogg/src/codebook.o \
        libogg/src/envelope.o \
        libogg/src/floor0.o \
        libogg/src/floor1.o \
        libogg/src/framing.o \
        libogg/src/info.o \
        libogg/src/lookup.o \
        libogg/src/lpc.o \
        libogg/src/lsp.o \
        libogg/src/mapping0.o \
        freetype/src/autofit/autofit.o \
        freetype/src/bdf/bdf.o \
        freetype/src/cff/cff.o \
        freetype/src/base/ftbase.o \
        freetype/src/base/ftbitmap.o \
        freetype/src/cache/ftcache.o \
        freetype/src/base/ftdebug.o \
        freetype/src/base/ftfstype.o \
        freetype/src/base/ftgasp.o \
        freetype/src/base/ftglyph.o \
        freetype/src/gzip/ftgzip.o \
        freetype/src/base/ftinit.o \
        freetype/src/lzw/ftlzw.o \
        freetype/src/base/ftstroke.o \
        freetype/src/base/ftsystem.o \
        freetype/src/smooth/smooth.o \
        freetype/src/base/ftbbox.o \
        freetype/src/base/ftfntfmt.o \
        freetype/src/base/ftgxval.o \
        freetype/src/base/ftlcdfil.o \
        freetype/src/base/ftmm.o \
        freetype/src/base/ftotval.o \
        freetype/src/base/ftpatent.o \
        freetype/src/base/ftpfr.o \
        freetype/src/base/ftsynth.o \
        freetype/src/base/fttype1.o \
        freetype/src/base/ftwinfnt.o \
        freetype/src/pcf/pcf.o \
        freetype/src/pfr/pfr.o \
        freetype/src/psaux/psaux.o \
        freetype/src/pshinter/pshinter.o \
        freetype/src/psnames/psmodule.o \
        freetype/src/raster/raster.o \
        freetype/src/sfnt/sfnt.o \
        freetype/src/truetype/truetype.o \
        freetype/src/type1/type1.o \
        freetype/src/cid/type1cid.o \
        freetype/src/type42/type42.o \
        freetype/src/winfonts/winfnt.o \
        curl/amigaos.o \
        curl/asyn-ares.o \
        curl/asyn-thread.o \
        curl/base64.o \
        curl/conncache.o \
        curl/connect.o \
        curl/content_encoding.o \
        curl/cookie.o \
        curl/curl_addrinfo.o \
        curl/curl_des.o \
        curl/curl_endian.o \
        curl/curl_fnmatch.o \
        curl/curl_gethostname.o \
        curl/curl_gssapi.o \
        curl/curl_memrchr.o \
        curl/curl_multibyte.o \
        curl/curl_ntlm_core.o \
        curl/curl_ntlm_wb.o \
        curl/curl_rtmp.o \
        curl/curl_sasl.o \
        curl/curl_sspi.o \
        curl/curl_threads.o \
        curl/dict.o \
        curl/dotdot.o \
        curl/easy.o \
        curl/escape.o \
        curl/file.o \
        curl/fileinfo.o \
        curl/formdata.o \
        curl/ftp.o \
        curl/ftplistparser.o \
        curl/getenv.o \
        curl/getinfo.o \
        curl/gopher.o \
        curl/hash.o \
        curl/hmac.o \
        curl/hostasyn.o \
        curl/hostcheck.o \
        curl/hostip.o \
        curl/hostip4.o \
        curl/hostip6.o \
        curl/hostsyn.o \
        curl/http.o \
        curl/http_chunks.o \
        curl/http_digest.o \
        curl/http_negotiate.o \
        curl/http_ntlm.o \
        curl/http_proxy.o \
        curl/http2.o \
        curl/idn_win32.o \
        curl/if2ip.o \
        curl/inet_ntop.o \
        curl/inet_pton.o \
        curl/ldap.o \
        curl/llist.o \
        curl/md4.o \
        curl/memdebug.o \
        curl/multi.o \
        curl/netrc.o \
        curl/non-ascii.o \
        curl/nonblock.o \
        curl/nwlib.o \
        curl/nwos.o \
        curl/openldap.o \
        curl/parsedate.o \
        curl/pingpong.o \
        curl/pipeline.o \
        curl/progress.o \
        curl/rand.o \
        curl/rtsp.o \
        curl/select.o \
        curl/sendf.o \
        curl/share.o \
        curl/slist.o \
        curl/smb.o \
        curl/socks.o \
        curl/socks_gssapi.o \
        curl/socks_sspi.o \
        curl/speedcheck.o \
        curl/splay.o \
        curl/ssh.o \
        curl/strdup.o \
        curl/strcase.o \
        curl/strerror.o \
        curl/strtok.o \
        curl/strtoofft.o \
        curl/system_win32.o \
        curl/telnet.o \
        curl/tftp.o \
        curl/timeval.o \
        curl/transfer.o \
        curl/url.o \
        curl/warnless.o \
        curl/wildcard.o \
        curl/x509asn1.o \
        curl/vtls/axtls.o \
        curl/vtls/cyassl.o \
        curl/vtls/darwinssl.o \
        curl/vtls/gskit.o \
        curl/vtls/gtls.o \
        curl/vtls/mbedtls.o \
        curl/vtls/nss.o \
        curl/vtls/openssl.o \
        curl/vtls/polarssl.o \
        curl/vtls/polarssl_threadlock.o \
        curl/vtls/schannel.o \
        curl/vtls/vtls.o \
        curl/imap.o \
        curl/mprintf.o \
        curl/md5.o \
        curl/pop3.o \
        curl/smtp.o \
        curl/version.o \
        curl/mbedTLS/xtea.o \
        curl/mbedTLS/aes.o \
        curl/mbedTLS/aesni.o \
        curl/mbedTLS/arc4.o \
        curl/mbedTLS/aria.o \
        curl/mbedTLS/asn1parse.o \
        curl/mbedTLS/asn1write.o \
        curl/mbedTLS/base64.o \
        curl/mbedTLS/bignum.o \
        curl/mbedTLS/blowfish.o \
        curl/mbedTLS/camellia.o \
        curl/mbedTLS/ccm.o \
        curl/mbedTLS/certs.o \
        curl/mbedTLS/chacha20.o \
        curl/mbedTLS/chachapoly.o \
        curl/mbedTLS/cipher.o \
        curl/mbedTLS/cipher_wrap.o \
        curl/mbedTLS/cmac.o \
        curl/mbedTLS/ctr_drbg.o \
        curl/mbedTLS/debug.o \
        curl/mbedTLS/des.o \
        curl/mbedTLS/dhm.o \
        curl/mbedTLS/ecdh.o \
        curl/mbedTLS/ecdsa.o \
        curl/mbedTLS/ecjpake.o \
        curl/mbedTLS/ecp.o \
        curl/mbedTLS/ecp_curves.o \
        curl/mbedTLS/entropy.o \
        curl/mbedTLS/entropy_poll.o \
        curl/mbedTLS/error.o \
        curl/mbedTLS/gcm.o \
        curl/mbedTLS/havege.o \
        curl/mbedTLS/hkdf.o \
        curl/mbedTLS/hmac_drbg.o \
        curl/mbedTLS/md.o \
        curl/mbedTLS/md_wrap.o \
        curl/mbedTLS/md2.o \
        curl/mbedTLS/md4.o \
        curl/mbedTLS/md5.o \
        curl/mbedTLS/memory_buffer_alloc.o \
        curl/mbedTLS/net_sockets.o \
        curl/mbedTLS/nist_kw.o \
        curl/mbedTLS/oid.o \
        curl/mbedTLS/padlock.o \
        curl/mbedTLS/pem.o \
        curl/mbedTLS/pk.o \
        curl/mbedTLS/pk_wrap.o \
        curl/mbedTLS/pkcs5.o \
        curl/mbedTLS/pkcs11.o \
        curl/mbedTLS/pkcs12.o \
        curl/mbedTLS/pkparse.o \
        curl/mbedTLS/pkwrite.o \
        curl/mbedTLS/platform.o \
        curl/mbedTLS/platform_util.o \
        curl/mbedTLS/poly1305.o \
        curl/mbedTLS/ripemd160.o \
        curl/mbedTLS/rsa.o \
        curl/mbedTLS/rsa_internal.o \
        curl/mbedTLS/sha1.o \
        curl/mbedTLS/sha256.o \
        curl/mbedTLS/sha512.o \
        curl/mbedTLS/ssl_cache.o \
        curl/mbedTLS/ssl_ciphersuites.o \
        curl/mbedTLS/ssl_cli.o \
        curl/mbedTLS/ssl_cookie.o \
        curl/mbedTLS/ssl_srv.o \
        curl/mbedTLS/ssl_ticket.o \
        curl/mbedTLS/ssl_tls.o \
        curl/mbedTLS/threading.o \
        curl/mbedTLS/timing.o \
        curl/mbedTLS/version.o \
        curl/mbedTLS/version_features.o \
        curl/mbedTLS/x509.o \
        curl/mbedTLS/x509_create.o \
        curl/mbedTLS/x509_crl.o \
        curl/mbedTLS/x509_crt.o \
        curl/mbedTLS/x509_csr.o \
        curl/mbedTLS/x509write_crt.o \
        curl/mbedTLS/x509write_csr.o \
        curl/vauth/cleartext.o \
        curl/vauth/cram.o \
        curl/vauth/digest.o \
        curl/vauth/digest_sspi.o \
        curl/vauth/krb5_gssapi.o \
        curl/vauth/krb5_sspi.o \
        curl/vauth/ntlm.o \
        curl/vauth/ntlm_sspi.o \
        curl/vauth/oauth2.o \
        curl/vauth/spnego_gssapi.o \
        curl/vauth/spnego_sspi.o \
        curl/vauth/vauth.o

        
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

all: setup $(OBJS) linkall

trial: trialflags all
	
trialflags:
	$(eval CFLAGS += -DFREEVERSION)
	$(eval OUT = $(OUT$(ARCH))Trial/libAGKLinux.a)
	mkdir -p $(OUT$(ARCH))Trial

setup:
	mkdir -p $(OUT$(ARCH))/obj/zxing/common/detector
	mkdir -p $(OUT$(ARCH))/obj/zxing/common/reedsolomon
	mkdir -p $(OUT$(ARCH))/obj/zxing/multi/qrcode/detector
	mkdir -p $(OUT$(ARCH))/obj/zxing/oned
	mkdir -p $(OUT$(ARCH))/obj/zxing/qrcode/decoder
	mkdir -p $(OUT$(ARCH))/obj/zxing/qrcode/detector
	mkdir -p $(OUT$(ARCH))/obj/zxing/datamatrix/decoder
	mkdir -p $(OUT$(ARCH))/obj/zxing/datamatrix/detector
	mkdir -p $(OUT$(ARCH))/obj/Collision
	mkdir -p $(OUT$(ARCH))/obj/Box2D/Dynamics/Contacts
	mkdir -p $(OUT$(ARCH))/obj/Box2D/Dynamics/Joints
	mkdir -p $(OUT$(ARCH))/obj/Box2D/Common
	mkdir -p $(OUT$(ARCH))/obj/Box2D/Collision/Shapes
	mkdir -p $(OUT$(ARCH))/obj/Box2D/Rope
	mkdir -p $(OUT$(ARCH))/obj/zip
	mkdir -p $(OUT$(ARCH))/obj/zlib
	mkdir -p $(OUT$(ARCH))/obj/libpng
	mkdir -p $(OUT$(ARCH))/obj/libjpeg
	mkdir -p $(OUT$(ARCH))/obj/giflib
	mkdir -p $(OUT$(ARCH))/obj/assimp/include/ConvertUTF
	mkdir -p $(OUT$(ARCH))/obj/assimp/include/irrXML
	mkdir -p $(OUT$(ARCH))/obj/assimp/include/poly2tri/common/
	mkdir -p $(OUT$(ARCH))/obj/assimp/include/poly2tri/sweep/
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletCollision/BroadphaseCollision
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletCollision/CollisionDispatch
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletCollision/CollisionShapes
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletCollision/Gimpact
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletCollision/NarrowPhaseCollision
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletDynamics/Character
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletDynamics/ConstraintSolver
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletDynamics/Dynamics
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletDynamics/Featherstone
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletDynamics/MLCPSolvers
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletDynamics/Vehicle
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletFileLoader
	mkdir -p $(OUT$(ARCH))/obj/bullet/BulletWorldImporter
	mkdir -p $(OUT$(ARCH))/obj/bullet/LinearMath
	mkdir -p $(OUT$(ARCH))/obj/libogg/src
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/autofit
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/bdf
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/base
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/cff
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/cache
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/gzip
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/lzw
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/smooth
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/pcf
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/pfr
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/psaux
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/pshinter
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/psnames
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/raster
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/sfnt
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/truetype
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/type1
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/cid
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/type42
	mkdir -p $(OUT$(ARCH))/obj/freetype/src/winfonts
	mkdir -p $(OUT$(ARCH))/obj/curl
	mkdir -p $(OUT$(ARCH))/obj/curl/mbedTLS
	mkdir -p $(OUT$(ARCH))/obj/curl/vauth
	mkdir -p $(OUT$(ARCH))/obj/curl/vtls

$(ODIR)/%.o: common/Source/%.cpp
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CPPFLAGS)
	
$(ODIR)/%.o: common/%.cpp
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CPPFLAGS)
	
$(ODIR)/%.o: platform/linux/Source/%.cpp
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CPPFLAGS)
	
$(ODIR)/%.o: renderer/OpenGL2/%.cpp
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CPPFLAGS)
	
$(ODIR)/zlib/%.o: zlib/%.c
	gcc -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CFLAGS)
	
$(ODIR)/libpng/%.o: libpng/%.c
	gcc -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CFLAGS)
	
$(ODIR)/libjpeg/%.o: libjpeg/%.c
	gcc -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CFLAGS)
	
$(ODIR)/giflib/%.o: giflib/%.c
	$(CC) -DAGK_LINUX -Wno-unused-result -DHAVE_CONFIG_H -c $(INC) -o $@ $< $(CFLAGS)
	
$(ODIR)/zxing/%.o: zxing/%.cpp
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CPPFLAGS)
	
$(ODIR)/assimp/%.o: assimp/%.cpp
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CPPFLAGS)
	
$(ODIR)/assimp/%.o: assimp/%.c
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CFLAGS)

$(ODIR)/assimp/%.o: assimp/%.cc
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CFLAGS)
	
$(ODIR)/bullet/%.o: bullet/%.cpp
	$(CC) -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CPPFLAGS)
	
$(ODIR)/libogg/%.o: libogg/%.c
	gcc -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CFLAGS)
	
$(ODIR)/freetype/%.o: freetype/%.c
	gcc -DAGK_LINUX -Wno-unused-result -c $(INC) -o $@ $< $(CFLAGS)
	
$(ODIR)/curl/%.o: curl/%.c
	gcc -DAGK_LINUX -DBUILDING_LIBCURL -DUSE_MBEDTLS -Wno-unused-result -c $(INC) -o $@ $< $(CFLAGS)
	
linkall: | $(OBJS)
	ar rcs $(OUT) $(OBJS)

clean:
	rm -rf $(OUT$(ARCH))/obj/*

