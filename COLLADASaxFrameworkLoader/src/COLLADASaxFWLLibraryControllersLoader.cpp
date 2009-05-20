/*
    Copyright (c) 2008-2009 NetAllied Systems GmbH

    This file is part of COLLADASaxFrameworkLoader.

    Licensed under the MIT Open Source License, 
    for details please see LICENSE file or the website
    http://www.opensource.org/licenses/mit-license.php
*/

#include "COLLADASaxFWLStableHeaders.h"
#include "COLLADASaxFWLLibraryControllersLoader.h"
#include "COLLADASaxFWLLoader.h"
#include "COLLADASaxFWLFileLoader.h"

#include "COLLADAFWSkinControllerData.h"
#include "COLLADAFWMorphController.h"
#include "COLLADAFWGeometry.h"
#include "COLLADAFWIWriter.h"
#include "COLLADAFWValidate.h"


namespace COLLADASaxFWL
{
	enum ControllerInputSemantics
	{
		SEMANTIC_UNKNOWN,
		SEMANTIC_JOINT,
		SEMANTIC_INV_BIND_MATRIX,
		SEMANTIC_WEIGHT,
		SEMANTIC_MORPH_TARGET,
		SEMANTIC_MORPH_WEIGHT
	};

	//------------------------------
	ControllerInputSemantics getControllerInputSemanticsBySemanticStr( const char * semanticString)
	{
		if ( strcmp(semanticString, "JOINT" ) == 0 )
		{
			return SEMANTIC_JOINT;
		}
		else if ( strcmp(semanticString, "INV_BIND_MATRIX" ) == 0 )
		{
			return SEMANTIC_INV_BIND_MATRIX;
		}
		else if ( strcmp(semanticString, "WEIGHT" ) == 0 )
		{
			return SEMANTIC_WEIGHT;
		}
		else if ( strcmp(semanticString, "MORPH_TARGET" ) == 0 )
		{
			return SEMANTIC_MORPH_TARGET;
		}
		else if ( strcmp(semanticString, "MORPH_WEIGHT" ) == 0 )
		{
			return SEMANTIC_MORPH_WEIGHT;
		}
		return SEMANTIC_UNKNOWN;
	}


    //------------------------------
	LibraryControllersLoader::LibraryControllersLoader( IFilePartLoader* callingFilePartLoader )
		: SourceArrayLoader(callingFilePartLoader)
		, mCurrentControllerType( UNKNOWN_CONTROLLER )
		, mCurrentSkinControllerData(0)
		, mCurrentMorphController(0)
		, mCurrentInputParent( INPUT_PARENT_UNKNOWN )
		, mJointSids( 0 )
		, mCurrentJointsVertexPairCount( 0 )
		, mJointOffset(0)
		, mWeightsOffset(0)
		, mCurrentMaxOffset(0)
		, mCurrentOffset(0)
		, mCurrentBindShapeMatrix( COLLADABU::Math::Matrix4::IDENTITY)
		, mCurrentMatrixIndex(0)
	{}

    //------------------------------
	LibraryControllersLoader::~LibraryControllersLoader()
	{
	}

	//------------------------------
	bool LibraryControllersLoader::writeVIndices ( const sint64* data, size_t length )
	{
		// check, if we are parsing an unsupported primitive type
		if ( !mCurrentSkinControllerData )
			return true;

		// Write the index values in the index lists.
		for ( size_t i=0; i<length; ++i )
		{
			// Get the current index value.
			unsigned int index = (unsigned int)data [i];

			// Write the indices
			if (  mCurrentOffset == mJointOffset )
			{
				COLLADAFW::UIntValuesArray& jointIndices = mCurrentSkinControllerData->getJointIndices();
				jointIndices.append ( index );
			}

			if ( mCurrentOffset == mWeightsOffset )
			{
				COLLADAFW::UIntValuesArray& weightIndices = mCurrentSkinControllerData->getWeightIndices();
				weightIndices.append ( index );
			}

			// Reset the offset if we went through all offset values
			if ( mCurrentOffset == mCurrentMaxOffset )
			{
				// Reset the current offset value
				mCurrentOffset = 0;
			}
			else
			{
				// Increment the current offset value
				++mCurrentOffset;
			}
		}
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::beginJointsArray()
	{
		const String& sourceId = getCurrentSourceId();

		// Without id we will never be able to us this array again. Don't store it.
		if ( sourceId.empty() )
			return true;

		mJointSids = &mJointSidsMap[sourceId];

		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::dataJointArray( const ParserString* data, size_t length )
	{
		if ( !mJointSids )
			return true;

		for ( size_t i = 0; i < length;  ++i)
		{
			const ParserString& parserString = data[i];
			mJointSids->push_back( String( parserString.str, parserString.length ) );
		}
		return true;
	}


	//------------------------------
	bool LibraryControllersLoader::end__library_controllers()
	{
		SaxVirtualFunctionTest(end__library_controllers());
		finish();
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::begin__controller( const controller__AttributeData& attributeData )
	{
		SaxVirtualFunctionTest(begin__controller(attributeData));
		if ( attributeData.id )
		{
			mCurrentControllerId = attributeData.id;
		}

		if ( attributeData.name )
		{
			mCurrentControllerName = attributeData.name;
		}
		else if ( attributeData.id )
		{
			mCurrentControllerName = attributeData.id;
		}

		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::end__controller()
	{
		SaxVirtualFunctionTest(end__controller());
		mCurrentControllerId.clear();
		mCurrentControllerName.clear();
		mCurrentMatrixIndex = 0;
		return true;
	}


	//------------------------------
	bool LibraryControllersLoader::begin__skin( const skin__AttributeData& attributeData )
	{
		SaxVirtualFunctionTest(begin__skin(attributeData));
		mCurrentControllerType = SKIN_CONTROLLER;
		mCurrentSkinControllerData = FW_NEW COLLADAFW::SkinControllerData(getUniqueIdFromId(mCurrentControllerId.c_str(), COLLADAFW::SkinControllerData::ID()).getObjectId());

		mCurrentControllerSourceUniqueId = getUniqueIdFromUrl(attributeData.source);
		addSkinDataSkinSourcePair( mCurrentSkinControllerData->getUniqueId(), attributeData.source);
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::end__skin()
	{
		SaxVirtualFunctionTest(end__skin());

		bool success = true;
		if ( validate( mCurrentSkinControllerData ) )
		{
			success = writer()->writeSkinControllerData( mCurrentSkinControllerData );
		}

		FW_DELETE mCurrentSkinControllerData;
		mCurrentSkinControllerData  = 0;
		mJointSidsMap.clear();
		mJointSids = 0;
		mCurrentControllerSourceUniqueId = COLLADAFW::UniqueId::INVALID;
		mCurrentControllerType = UNKNOWN_CONTROLLER;
		return success;
	}

	//------------------------------
	bool LibraryControllersLoader::begin__morph( const morph__AttributeData& attributeData )
	{
		SaxVirtualFunctionTest(begin__morph(attributeData));
		mCurrentControllerType = MORPH_CONTROLLER;
		mCurrentMorphController = FW_NEW COLLADAFW::MorphController(getUniqueIdFromId(mCurrentControllerId.c_str(), COLLADAFW::MorphController::ID()).getObjectId());
		mCurrentControllerSourceUniqueId = getUniqueIdFromUrl(attributeData.source, COLLADAFW::Geometry::ID());
		mCurrentMorphController->setSource( mCurrentControllerSourceUniqueId);

		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::end__morph()
	{
		SaxVirtualFunctionTest(end__morph());
		if ( mCurrentMorphController )
		{
			addMorphController( mCurrentMorphController );
		}
		mJointSidsMap.clear();
		mJointSids = 0;
		mCurrentMorphController = 0;
		mCurrentControllerSourceUniqueId = COLLADAFW::UniqueId::INVALID;
		mCurrentControllerType = UNKNOWN_CONTROLLER;
		return true;
	}


	//------------------------------
	bool LibraryControllersLoader::begin__joints()
	{
		SaxVirtualFunctionTest(begin__joints());
		mCurrentInputParent = INPUT_PARENT_JOINTS;
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::begin__source( const source__AttributeData& attributes )
	{
		SaxVirtualFunctionTest(begin__source(attributes));
		return beginSource(attributes);
	}

	//------------------------------
	bool LibraryControllersLoader::end__source(  )
	{
		SaxVirtualFunctionTest(end__source());
		return endSource();
	}

	//------------------------------
	bool LibraryControllersLoader::end__joints()
	{
		SaxVirtualFunctionTest(end__joints());
		mCurrentInputParent = INPUT_PARENT_UNKNOWN;
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::begin__input____InputLocal( const input____InputLocal__AttributeData& attributeData )
	{
		SaxVirtualFunctionTest(begin__input____InputLocal(attributeData));
		
		// we ignore inputs that don't have semantics or source
		if ( !attributeData.semantic || !attributeData.source  )
		{
			return true;
		}

		ControllerInputSemantics semantic = getControllerInputSemanticsBySemanticStr( attributeData.semantic );
		if ( semantic == SEMANTIC_UNKNOWN )
		{
			return true;
		}

		switch ( mCurrentControllerType )
		{
		case SKIN_CONTROLLER:
			{
				switch ( semantic )
				{
				case SEMANTIC_JOINT:
					{
						if ( !mCurrentSkinControllerData )
						{
							break;
						}

						String sourceId = getIdFromURIFragmentType(attributeData.source);
						StringListMap::const_iterator it = mJointSidsMap.find(sourceId);

						// check if the node sid array could be found
						if ( it == mJointSidsMap.end() )
							break;

						const StringList& nodeSids = it->second;

						const COLLADAFW::UniqueId& controllerUniqueId = mCurrentSkinControllerData->getUniqueId();
						addSkinDataJointSidsPair( controllerUniqueId, nodeSids);

						// try to write the SkinController here
						if ( ((getObjectFlags() & Loader::CONTROLLER_FLAG) != 0) && (mCurrentControllerSourceUniqueId.isValid()) )
						{
							InstanceControllerDataList& instanceControllerDataList = getInstanceControllerDataListByControllerUniqueId(controllerUniqueId);
							InstanceControllerDataList::iterator listIt = instanceControllerDataList.begin();

							while ( listIt != instanceControllerDataList.end() )
							{
								const InstanceControllerData& instanceControllerData = *listIt;
								bool success = getFileLoader()->createAndWriteSkinController( instanceControllerData, 
																					   		  controllerUniqueId, 
									                                                          mCurrentControllerSourceUniqueId,
									                                                          nodeSids);
								//on success we need to remove this controller instance
								if ( success )
								{
									listIt = instanceControllerDataList.erase( listIt );
								}
								else
								{
									listIt++;
								}
							}
						}
						mCurrentSkinControllerData->setJointsCount(nodeSids.size());
					}
					break;
				case SEMANTIC_INV_BIND_MATRIX:
					{
						if ( !mCurrentSkinControllerData)
						{
							break;
						}

						String sourceId = getIdFromURIFragmentType(attributeData.source);
						SourceBase* sourceBase = getSourceById ( sourceId );

						if ( !sourceBase || (sourceBase->getDataType() != SourceBase::DATA_TYPE_REAL) )
						{
							// TODO handle error
							break;
						}

						if ( sourceBase->getStride() != 16 )
						{
							// TODO handle error
							break;
						}

						const RealSource *inverseBindMatricesSource = (const RealSource *)sourceBase;
						const RealArrayElement& inverseBindMatricesElement = inverseBindMatricesSource->getArrayElement();

						const RealArray& inverseBindMatricesArray = inverseBindMatricesElement.getValues();

						size_t matrixElementsCount = inverseBindMatricesArray.getCount();

						size_t matrixCount = matrixElementsCount / 16;


						COLLADAFW::Matrix4Array& inverseBindMatrices = mCurrentSkinControllerData->getInverseBindMatrices();
						inverseBindMatrices.allocMemory( matrixCount );
						inverseBindMatrices.setCount( matrixCount );

						size_t index = 0;
						for ( size_t i = 0; i < matrixCount; ++i)
						{
							// fill the matrix
							COLLADABU::Math::Matrix4 matrix;
							for ( size_t j = 0; j < 16; ++j,++index)
							{
								matrix.setElement( j, inverseBindMatricesArray[index]);
							}
							inverseBindMatrices[i] = matrix;
						}
					}
					break;
				}
			}
			break;
		case MORPH_CONTROLLER:
			{
				switch ( semantic )
				{
				case SEMANTIC_MORPH_TARGET:
					{
						if ( !mCurrentMorphController )
						{
							break;
						}

						String sourceId = getIdFromURIFragmentType(attributeData.source);
						StringListMap::const_iterator it = mJointSidsMap.find(sourceId);

						// check if the node sid array could be found
						if ( it == mJointSidsMap.end() )
							break;

						const StringList& meshIds = it->second;
						size_t meshIdCount = meshIds.size();

						COLLADAFW::UniqueIdArray& morphTargets = mCurrentMorphController->getMorphTargets();
						morphTargets.allocMemory(meshIdCount);
						morphTargets.setCount(meshIdCount);
						StringList::const_iterator itTarget = meshIds.begin();
						for ( size_t i = 0 ; itTarget != meshIds.end(); ++itTarget, ++i)
						{
							morphTargets[i] = getUniqueIdFromId( itTarget->c_str(), COLLADAFW::Geometry::ID());
						}

					}
					break;
				case SEMANTIC_MORPH_WEIGHT:
					{
						if ( !mCurrentMorphController)
						{
							break;
						}

						String sourceId = getIdFromURIFragmentType(attributeData.source);
						SourceBase* sourceBase = getSourceById( sourceId );

						if ( !sourceBase || (sourceBase->getDataType() != SourceBase::DATA_TYPE_REAL) )
						{
							// TODO handle error
							break;
						}

						if ( sourceBase->getStride() != 1 )
						{
							// TODO handle error
							break;
						}
						const RealSource *weightSource = (const RealSource *)sourceBase;
						COLLADAFW::FloatOrDoubleArray& morphWeights = mCurrentMorphController->getMorphWeights();
						addToSidTree( sourceId.c_str(), 0, &morphWeights );
						moveUpInSidTree();

						setRealValues( morphWeights, weightSource );
					}
					break;
				}
			}
			break;
		}

		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::begin__vertex_weights( const vertex_weights__AttributeData& attributeData )
	{
		SaxVirtualFunctionTest(begin__vertex_weights(attributeData));
		mCurrentInputParent = INPUT_PARENT_VERTEX_WEIGHTS;
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::end__vertex_weights()
	{
		SaxVirtualFunctionTest(end__vertex_weights());
		mCurrentInputParent = INPUT_PARENT_UNKNOWN;
		mJointOffset = 0;
		mWeightsOffset = 0;
		mCurrentMaxOffset = 0;
		mCurrentOffset = 0;
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::begin__input____InputLocalOffset( const input____InputLocalOffset__AttributeData& attributeData )
	{
		SaxVirtualFunctionTest(begin__input____InputLocalOffset(attributeData));

		if ( attributeData.offset > mCurrentMaxOffset )
		{
			mCurrentMaxOffset = attributeData.offset;
		}

		// we ignore inputs that don't have semantics or source
		if ( !attributeData.semantic || !attributeData.source  )
		{
			return true;
		}

		ControllerInputSemantics semantic = getControllerInputSemanticsBySemanticStr( attributeData.semantic );
		if ( semantic == SEMANTIC_UNKNOWN )
		{
			return true;
		}

		String sourceId = getIdFromURIFragmentType(attributeData.source);
		SourceBase* sourceBase = getSourceById ( sourceId );

		switch ( semantic )
		{
		case SEMANTIC_WEIGHT:
			{
				mWeightsOffset = attributeData.offset;

				if ( !mCurrentSkinControllerData ||  !sourceBase || (sourceBase->getDataType() != SourceBase::DATA_TYPE_REAL) )
					break;

				COLLADAFW::FloatOrDoubleArray& weights = mCurrentSkinControllerData->getWeights();
				assignSourceValuesToFloatOrDoubleArray( sourceBase, weights );
				break;
			}
		case SEMANTIC_JOINT:
			mJointOffset = attributeData.offset;
			break;
		}
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::end__input____InputLocalOffset()
	{
		SaxVirtualFunctionTest(end__input____InputLocalOffset());
		return true;
	}


	//------------------------------
	bool LibraryControllersLoader::begin__vcount()
	{
		SaxVirtualFunctionTest(begin__vcount());
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::end__vcount()
	{
		SaxVirtualFunctionTest(end__vcount());
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::data__vcount( const unsigned long long* data, size_t length )
	{
		SaxVirtualFunctionTest(data__vcount(data, length));
		if ( !mCurrentSkinControllerData )
			return true;
		COLLADAFW::UIntValuesArray& jointsPerVertex = mCurrentSkinControllerData->getJointsPerVertex();
		size_t count = jointsPerVertex.getCount();
		jointsPerVertex.reallocMemory( count + length);
		for ( size_t i = 0; i < length; ++i)
		{
			unsigned long long vcount = data[i];
			jointsPerVertex.append((unsigned int)vcount);
			mCurrentJointsVertexPairCount += (size_t)vcount;
		}
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::begin__v()
	{
		SaxVirtualFunctionTest(begin__v());
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::end__v()
	{
		SaxVirtualFunctionTest(end__v());
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::data__v( const sint64* data, size_t length )
	{
		SaxVirtualFunctionTest(data__v(data, length));
		writeVIndices(data, length);
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::begin__Name_array( const Name_array__AttributeData& attributeData )
	{
		SaxVirtualFunctionTest(begin__Name_array(attributeData));
		return beginJointsArray();
	}

	//------------------------------
	bool LibraryControllersLoader::end__Name_array()
	{
		SaxVirtualFunctionTest(end__Name_array());
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::data__Name_array( const ParserString* data, size_t length )
	{
		SaxVirtualFunctionTest(data__Name_array(data, length));
		return dataJointArray( data, length);
	}

	//------------------------------
	bool LibraryControllersLoader::begin__IDREF_array( const IDREF_array__AttributeData& attributeData )
	{
		SaxVirtualFunctionTest(begin__IDREF_array(attributeData));
		return beginJointsArray();
	}

	//------------------------------
	bool LibraryControllersLoader::end__IDREF_array()
	{
		SaxVirtualFunctionTest(end__IDREF_array());
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::data__IDREF_array( const ParserString* data, size_t length )
	{
		SaxVirtualFunctionTest(data__IDREF_array(data, length));
		return dataJointArray( data, length);
	}

	//------------------------------
	bool LibraryControllersLoader::data__bind_shape_matrix( const float* data, size_t length )
	{
		SaxVirtualFunctionTest(data__bind_shape_matrix(data, length));
		for ( size_t i = 0; i < length; ++i)
		{
			size_t row = mCurrentMatrixIndex / 4;
			size_t column = mCurrentMatrixIndex % 4;
			mCurrentBindShapeMatrix.setElement(row, column, data[i]);
			mCurrentMatrixIndex++;
		}
		return true;
	}

	//------------------------------
	bool LibraryControllersLoader::end__bind_shape_matrix()
	{
		SaxVirtualFunctionTest(end__bind_shape_matrix());
		if (mCurrentSkinControllerData)
		{
			mCurrentSkinControllerData->setBindShapeMatrix( mCurrentBindShapeMatrix );
		}
		return true;
	}

} // namespace COLLADASaxFWL
