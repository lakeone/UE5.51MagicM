// Copyright Epic Games, Inc. All Rights Reserved.

#include "MuT/ASTOpMeshApplyPose.h"
#include "MuT/ASTOpMeshAddTags.h"

#include "HAL/PlatformMath.h"
#include "MuR/ModelPrivate.h"
#include "MuR/RefCounted.h"
#include "MuR/Types.h"

namespace mu
{


	//-------------------------------------------------------------------------------------------------
	ASTOpMeshApplyPose::ASTOpMeshApplyPose()
		: base(this)
		, pose(this)
	{
	}


	ASTOpMeshApplyPose::~ASTOpMeshApplyPose()
	{
		// Explicit call needed to avoid recursive destruction
		ASTOp::RemoveChildren();
	}


	bool ASTOpMeshApplyPose::IsEqual(const ASTOp& otherUntyped) const
	{
		if (otherUntyped.GetOpType()==GetOpType())
		{
			const ASTOpMeshApplyPose* other = static_cast<const ASTOpMeshApplyPose*>(&otherUntyped);
			return base == other->base &&
				pose == other->pose;
		}
		return false;
	}


	uint64 ASTOpMeshApplyPose::Hash() const
	{
		uint64 res = std::hash<void*>()(base.child().get());
		hash_combine(res, pose.child().get());
		return res;
	}


	mu::Ptr<ASTOp> ASTOpMeshApplyPose::Clone(MapChildFuncRef mapChild) const
	{
		Ptr<ASTOpMeshApplyPose> n = new ASTOpMeshApplyPose();
		n->base = mapChild(base.child());
		n->pose = mapChild(pose.child());
		return n;
	}


	void ASTOpMeshApplyPose::ForEachChild(const TFunctionRef<void(ASTChild&)> f)
	{
		f(base);
		f(pose);
	}


	void ASTOpMeshApplyPose::Link(FProgram& program, FLinkerOptions*)
	{
		// Already linked?
		if (!linkedAddress)
		{
			OP::MeshApplyPoseArgs args;
			memset(&args, 0, sizeof(args));

			if (base) args.base = base->linkedAddress;
			if (pose) args.pose = pose->linkedAddress;

			linkedAddress = (OP::ADDRESS)program.m_opAddress.Num();
			//program.m_code.push_back(op);
			program.m_opAddress.Add((uint32_t)program.m_byteCode.Num());
			AppendCode(program.m_byteCode, OP_TYPE::ME_APPLYPOSE);
			AppendCode(program.m_byteCode, args);
		}

	}

	Ptr<ASTOp> ASTOpMeshApplyPose::OptimiseSink(const FModelOptimizationOptions&, FOptimizeSinkContext&) const
	{
		Ptr<ASTOp> NewOp;

		Ptr<ASTOp> MeshAt = base.child();

		if (!MeshAt)
		{
			return nullptr;
		}

		OP_TYPE MeshType = MeshAt->GetOpType();

		switch (MeshType)
		{
			case OP_TYPE::ME_ADDTAGS:
			{
				Ptr<ASTOpMeshAddTags> New = mu::Clone<ASTOpMeshAddTags>(MeshAt);
				if (New->Source)
				{
					Ptr<ASTOpMeshApplyPose> NewApplyPose = mu::Clone<ASTOpMeshApplyPose>(this);
					NewApplyPose->base = New->Source.child();
					New->Source = NewApplyPose;
				}

				NewOp = New;
				break;
			}

			default:
				break;
		}

		return NewOp;
	}

	FSourceDataDescriptor ASTOpMeshApplyPose::GetSourceDataDescriptor(FGetSourceDataDescriptorContext* Context) const
	{
		if (base)
		{
			return base->GetSourceDataDescriptor(Context);
		}

		return {};
	}
}
