// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MuR/Image.h"
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

    class NodeImageMipmap;
    typedef Ptr<NodeImageMipmap> NodeImageMipmapPtr;
    typedef Ptr<const NodeImageMipmap> NodeImageMipmapPtrConst;


	//! Generate mimaps for an image.
	//! \ingroup model
    class MUTABLETOOLS_API NodeImageMipmap : public NodeImage
	{
	public:

        NodeImageMipmap();

		//-----------------------------------------------------------------------------------------
		// Node Interface
		//-----------------------------------------------------------------------------------------

        const FNodeType* GetType() const override;
		static const FNodeType* GetStaticType();

		//-----------------------------------------------------------------------------------------
		// Own Interface
		//-----------------------------------------------------------------------------------------

        //! Get the node generating the source image to be mipmapped.
		NodeImagePtr GetSource() const;
		void SetSource( NodeImagePtr );

		void SetMipmapGenerationSettings(EMipmapFilterType FilterType, EAddressMode AddressMode);

		//-----------------------------------------------------------------------------------------
		// Interface pattern
		//-----------------------------------------------------------------------------------------
		class Private;
		Private* GetPrivate() const;

	protected:

		//! Forbidden. Manage with the Ptr<> template.
        ~NodeImageMipmap();

	private:

		Private* m_pD;

	};


}
