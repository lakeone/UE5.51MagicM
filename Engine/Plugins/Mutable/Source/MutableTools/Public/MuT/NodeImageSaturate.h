// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MuR/Ptr.h"
#include "MuR/RefCounted.h"
#include "MuT/Node.h"
#include "MuT/NodeImage.h"


namespace mu
{

	// Forward definitions
	class NodeScalar;
	typedef Ptr<NodeScalar> NodeScalarPtr;
	typedef Ptr<const NodeScalar> NodeScalarPtrConst;

	class NodeImageSaturate;
	typedef Ptr<NodeImageSaturate> NodeImageSaturatePtr;
	typedef Ptr<const NodeImageSaturate> NodeImageSaturatePtrConst;


	//! Change the saturation of an image. This node can be used to increase the saturation with a
	//! factor bigger than 1, or to decrease it or desaturate it completely with a factor smaller
	//! than 1 or 0.
	class MUTABLETOOLS_API NodeImageSaturate : public NodeImage
	{
	public:

		NodeImageSaturate();

		//-----------------------------------------------------------------------------------------
		// Node Interface
		//-----------------------------------------------------------------------------------------

        const FNodeType* GetType() const override;
		static const FNodeType* GetStaticType();

		//-----------------------------------------------------------------------------------------
		// Own Interface
		//-----------------------------------------------------------------------------------------

		//! Get the node generating the saturation factor.
		//! A value of 0 completely desaturates. 1 will leave the same saturation and bigger than 1
		//! will increase it.
		NodeScalarPtr GetFactor() const;
		void SetFactor( NodeScalarPtr );

		//! Get the node generating the source image to be [de]saturated.
		NodeImagePtr GetSource() const;
		void SetSource( NodeImagePtr );

		//-----------------------------------------------------------------------------------------
		// Interface pattern
		//-----------------------------------------------------------------------------------------
		class Private;

		Private* GetPrivate() const;

	protected:

		//! Forbidden. Manage with the Ptr<> template.
		~NodeImageSaturate();

	private:

		Private* m_pD;

	};


}
