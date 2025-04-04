// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MuR/Image.h"
#include "MuR/Ptr.h"
#include "MuR/RefCounted.h"
#include "MuT/Node.h"
#include "MuT/NodeImage.h"
#include "MuT/NodeRange.h"


namespace mu
{

	// Forward definitions
    class NodeImageMultiLayer;
    typedef Ptr<NodeImageMultiLayer> NodeImageMultiLayerPtr;
    typedef Ptr<const NodeImageMultiLayer> NodeImageMultiLayerPtrConst;


    //! This node applies any numbre of layer blending effect on a base image using a mask and a
    //! blended image. The number of layers depends on a scalar input of this node.
	//! \ingroup model
    class MUTABLETOOLS_API NodeImageMultiLayer : public NodeImage
	{
	public:

        NodeImageMultiLayer();

		//-----------------------------------------------------------------------------------------
		// Node Interface
		//-----------------------------------------------------------------------------------------

        const FNodeType* GetType() const override;
		static const FNodeType* GetStaticType();

		//-----------------------------------------------------------------------------------------
		// Own Interface
		//-----------------------------------------------------------------------------------------

		//! Get the node generating the base image that will have the blendeing effect applied.
		NodeImagePtr GetBase() const;
		void SetBase( NodeImagePtr );

		//! Get the node generating the mask image controlling the weight of the effect.
		NodeImagePtr GetMask() const;
		void SetMask( NodeImagePtr );

        //! Get the image blended on top of the base.
        NodeImagePtr GetBlended() const;
        void SetBlended( NodeImagePtr );

        //! Get the number of layers to apply.
        NodeRangePtr GetRange() const;
        void SetRange( NodeRangePtr );

		//!
		EBlendType GetBlendType() const;
        void SetBlendType(EBlendType);

		//-----------------------------------------------------------------------------------------
		// Interface pattern
		//-----------------------------------------------------------------------------------------
		class Private;
		Private* GetPrivate() const;

	protected:

		//! Forbidden. Manage with the Ptr<> template.
        ~NodeImageMultiLayer();

	private:

		Private* m_pD;

	};


}
