// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MuR/Parameters.h"
#include "MuR/Ptr.h"
#include "MuR/RefCounted.h"
#include "MuT/Node.h"
#include "MuT/NodeString.h"


namespace mu
{

	// Forward definitions
    class NodeImage;
    using NodeImagePtr = Ptr<NodeImage>;
    using NodeImagePtrConst = Ptr<const NodeImage>;

    class NodeRange;
    using NodeRangePtr = Ptr<NodeRange>;
    using NodeRangePtrConst = Ptr<const NodeRange>;

    class NodeStringParameter;
    using NodeStringParameterPtr = Ptr<NodeStringParameter>;
    using NodeStringParameterPtrConst = Ptr<const NodeStringParameter>;


	//! Node that defines a string model parameter.
	class MUTABLETOOLS_API NodeStringParameter : public NodeString
	{
	public:

		NodeStringParameter();

		//-----------------------------------------------------------------------------------------
		// Node Interface
		//-----------------------------------------------------------------------------------------

        const FNodeType* GetType() const override;
		static const FNodeType* GetStaticType();

		//-----------------------------------------------------------------------------------------
		// Own Interface
		//-----------------------------------------------------------------------------------------

		//! Set the name of the parameter. It will be exposed in the final compiled data.
		void SetName( const FString& );

		//! Set the uid of the parameter. It will be exposed in the final compiled data.
		void SetUid( const FString& );

		//! Set the default value of the parameter.
		void SetDefaultValue( const FString& );

        //! Set the number of ranges (dimensions) for this parameter.
        //! By default a parameter has 0 ranges, meaning it only has one value.
        void SetRangeCount( int i );
        void SetRange( int i, NodeRangePtr pRange );

		//-----------------------------------------------------------------------------------------
		// Interface pattern
		//-----------------------------------------------------------------------------------------
		class Private;
		Private* GetPrivate() const;

	protected:

		//! Forbidden. Manage with the Ptr<> template.
		~NodeStringParameter();

	private:

		Private* m_pD;

	};


}
