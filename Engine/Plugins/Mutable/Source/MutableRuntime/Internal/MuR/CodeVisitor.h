// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HAL/PlatformMath.h"
#include "HAL/UnrealMemory.h"
#include "Misc/AssertionMacros.h"
#include "MuR/Model.h"
#include "MuR/ModelPrivate.h"
#include "MuR/MutableMemory.h"
#include "MuR/Operations.h"
#include "MuR/ParametersPrivate.h"
#include "MuR/Ptr.h"
#include "MuR/System.h"
#include "MuR/SystemPrivate.h"

namespace mu
{

    //---------------------------------------------------------------------------------------------
    //! Decide what operations are an "add resource" since they are handled differently sometimes.
    //---------------------------------------------------------------------------------------------
    inline bool VisitorIsAddResource(const OP_TYPE& type)
    {
        return  type == OP_TYPE::IN_ADDIMAGE ||
                type == OP_TYPE::IN_ADDMESH;
    }


    //---------------------------------------------------------------------------------------------
    //! Code visitor that:
    //! - is top-down
    //! - cannot change the visited instructions.
    //! - will not visit twice the same instruction with the same state.
    //! - Its iterative
    //---------------------------------------------------------------------------------------------
    template<typename STATE=int>
    class UniqueConstCodeVisitorIterative
    {
    public:

        UniqueConstCodeVisitorIterative( bool skipResources=false )
        {
            // Default state
            m_states.Add(STATE());
            m_currentState = 0;
            m_skipResources = skipResources;
        }

        //! Ensure virtual destruction
        virtual ~UniqueConstCodeVisitorIterative() {}

    protected:

        //!
        void SetDefaultState(const STATE& s)
        {
            m_states[0] = s;
        }

        //!
        const STATE& GetDefaultState() const
        {
            return m_states[0];
        }

        //! Use this from visit to access the state at the time of processing the current
        //! instruction.
        STATE GetCurrentState() const
        {
            return m_states[m_currentState];
        }

        //! For manual recursion that changes the state for a specific path.
        void RecurseWithState(OP::ADDRESS at, const STATE& newState)
        {
			int32 it = m_states.Find(newState);
            if (it==INDEX_NONE)
            {
                m_states.Add(newState);
            }
            int stateIndex = m_states.IndexOfByKey(newState);

            m_pending.Add( PENDING(at,stateIndex) );
        }

        //! For manual recursion that doesn't change the state for a specific path.
        void RecurseWithCurrentState(OP::ADDRESS at)
        {
            m_pending.Add( PENDING(at,m_currentState) );
        }

        //! Can be called from visit to set the state to visit all children ops
        void SetCurrentState(const STATE& newState)
        {
            int32 it = m_states.Find(newState);
            if (it==INDEX_NONE)
            {
                m_states.Add(newState);
            }
            m_currentState = m_states.Find(newState);
        }


        void Traverse( OP::ADDRESS root, FProgram& program )
        {
            m_pending.Reserve( program.m_opAddress.Num() );

            // Visit the given root
            m_pending.Add( PENDING(root,0) );
            Recurse( program );
         }

        void FullTraverse( FProgram& program )
        {
            // Visit all the state roots
            for ( int32 p=0; p<program.m_states.Num(); ++p )
            {
                m_pending.Add(PENDING(program.m_states[p].m_root,0) );
                Recurse( program );
            }
        }


    private:

        //! Do the actual work by overriding this in the derived classes.
        //! Return true if the traverse has to continue with the children of "at"
        virtual bool Visit( OP::ADDRESS at, FProgram& program ) = 0;

        //! Operations to be processed
        struct PENDING
        {
            PENDING()
            {
                at = 0;
                stateIndex = 0;
            }
            PENDING(OP::ADDRESS _at, int _stateIndex)
            {
                at = _at;
                stateIndex = _stateIndex;
            }

            OP::ADDRESS at;
            int stateIndex;
        };
		TArray<PENDING> m_pending;

        //! States found so far
		TArray<STATE> m_states;

        //! Index of the current state, from the m_states array.
        int m_currentState;

        //! If true, operations adding resources (meshes or images) will only
        //! have the base operation recursed, but not the resources.
        bool m_skipResources;

        //! Array of states visited for each operation.
        //! Empty array means operation not visited at all.
		TArray<TArray<int>> m_visited;

        //! Process all the pending operations and visit all children if necessary
        void Recurse( FProgram& program )
        {
			m_visited.Empty();
			m_visited.SetNum(program.m_opAddress.Num());

            while ( m_pending.Num() )
            {
                OP::ADDRESS at = m_pending.Last().at;
                m_currentState = m_pending.Last().stateIndex;
                m_pending.Pop();

                bool recurse = false;

                bool visitedInThisState = m_visited[at].Contains(m_currentState);
                if (!visitedInThisState)
                {
                    m_visited[at].Add(m_currentState);

                    // Visit may change current state
                    recurse = Visit( at, program );
                }

                if (recurse)
                {
                    ForEachReference( program, at, [&](OP::ADDRESS ref)
                    {
                        if (ref)
                        {
                            m_pending.Add( PENDING(ref,m_currentState) );
                        }
                    });
                }
            }
        }

    };


    //---------------------------------------------------------------------------------------------
    //! Code visitor that:
    //! - is top-down
    //! - cannot change the instructions.
    //! - will repeat visits to the instructions that have multiple references.
    //! - Its iterative
    //---------------------------------------------------------------------------------------------
    template<class STATE=int>
    class RepeatConstCodeVisitorIterative
    {
    public:

        RepeatConstCodeVisitorIterative( bool skipResources=false )
        {
            // Default state
            m_states.Add(STATE());
            m_currentState = 0;
            m_skipResources = skipResources;
        }

        //! Ensure virtual destruction
        virtual ~RepeatConstCodeVisitorIterative() {}

    protected:

        //!
        void SetDefaultState(const STATE& s)
        {
            m_states[0] = s;
        }

        //!
        const STATE& GetDefaultState() const
        {
            return m_states[0];
        }

        //! Use this from visit to access the state at the time of processing the current
        //! instruction.
        const STATE& GetCurrentState() const
        {
            return m_states[m_currentState];
        }

        //! For manual recursion that changes the state for a specific path.
        void RecurseWithState(OP::ADDRESS At, const STATE& NewState)
        {
            int32 StateIndex = m_states.Find(NewState);
            if (StateIndex ==INDEX_NONE)
            {
				StateIndex = m_states.Add(NewState);
            }

            //check(at<1000000);
            m_pending.Add( PENDING(At,StateIndex) );
        }

        //! For manual recursion that doesn't change the state for a specific path.
        void RecurseWithCurrentState(OP::ADDRESS at)
        {
            //check(at<1000000);
            m_pending.Add( PENDING(at,m_currentState) );
        }

        //! Can be called from visit to set the state to visit all children ops
        void SetCurrentState(const STATE& NewState)
        {
			int32 StateIndex = m_states.Find(NewState);
			if (StateIndex == INDEX_NONE)
			{
				StateIndex = m_states.Add(NewState);
			}

			m_currentState = StateIndex;
        }


        void Traverse( OP::ADDRESS Root, FProgram& Program )
        {
            m_pending.reserve(Program.m_opAddress.Num() );

            // Visit the given root
            m_pending.Add( PENDING(Root,0) );
            Recurse(Program);
        }

        void FullTraverse( FProgram& Program)
        {
            // Visit all the state roots
            for ( int32 p=0; p<Program.m_states.Num(); ++p )
            {
                m_pending.Add(PENDING(Program.m_states[p].m_root,0) );
                Recurse(Program);
            }
        }


    private:

        //! Do the actual work by overriding this in the derived classes.
        //! Return true if the traverse has to continue with the children of "at"
        virtual bool Visit( OP::ADDRESS at, FProgram& program ) = 0;

        //! Operations to be processed
        struct PENDING
        {
            PENDING()
            {
                at = 0;
                stateIndex = 0;
            }
            PENDING(OP::ADDRESS _at, int _stateIndex)
            {
                at = _at;
                stateIndex = _stateIndex;
            }

            OP::ADDRESS at;
            int stateIndex;
        };
        TArray<PENDING> m_pending;

        //! States found so far
		TArray<STATE> m_states;

        //! Index of the current state, from the m_states array.
        int m_currentState;

        //! If true, operations adding resources (meshes or images) will only
        //! have the base operation recursed, but not the resources.
        bool m_skipResources;


        //! Process all the pending operations and visit all children if necessary
        void Recurse( FProgram& program )
        {
            while ( m_pending.Num() )
            {
                OP::ADDRESS at = m_pending.back().at;
                m_currentState = m_pending.back().stateIndex;
                m_pending.pop_back();

                bool recurse = false;

                // Visit may change current state
                recurse = Visit( at, program );

                if (recurse)
                {
                    if (m_skipResources && VisitorIsAddResource(program.GetOpType(at)))
                    {
						OP::InstanceAddArgs args = program.GetOpArgs<OP::InstanceAddArgs>(at);

                        // Recurse only the base
                        OP::ADDRESS base = args.instance;
                        if (base)
                        {
							check(base<program.m_opAddress.Num());
                            m_pending.Add( PENDING(base,m_currentState) );
                        }
                    }
                    else
                    {
                        ForEachReference( program, at, [&](OP::ADDRESS ref)
                        {
                            if (ref)
                            {
								check(ref<program.m_opAddress.Num());
                                m_pending.Add( PENDING(ref,m_currentState) );
                            }
                        });
                    }
                }
            }
        }

    };


    //---------------------------------------------------------------------------------------------
    //! Code visitor template for visitors that:
    //! - only traverses the operations that are relevant for a given set of parameter values. It
    //! only considers the discrete parameters like integers and booleans. In the case of forks
    //! caused by continuous parameters like float weights for interpolation, all the branches are
    //! traversed.
    //! - cannot change the instructinos
    //---------------------------------------------------------------------------------------------
    struct COVERED_CODE_VISITOR_STATE
    {
        uint16 m_underResourceCount = 0;

        bool operator==(const COVERED_CODE_VISITOR_STATE& o) const
        {
            return m_underResourceCount==o.m_underResourceCount;
        }
    };

    template<typename PARENT,typename STATE>
    class DiscreteCoveredCodeVisitorBase : public PARENT
    {
    public:

        DiscreteCoveredCodeVisitorBase
            (
                System::Private* pSystem,
                const TSharedPtr<const Model>& pModel,
                const ParametersPtrConst& pParams,
                unsigned lodMask,
                bool skipResources=false
            )
            : PARENT(skipResources)
        {
            m_pSystem = pSystem;
            m_pModel = pModel;
            m_pParams = pParams.get();
            m_lodMask = lodMask;

            // Visiting state
            PARENT::SetDefaultState( STATE() );
        }

        void Run( OP::ADDRESS at  )
        {
            PARENT::SetDefaultState( STATE() );

            PARENT::Traverse( at, m_pModel->GetPrivate()->m_program );
        }

    protected:

        virtual bool Visit( OP::ADDRESS at, FProgram& program )
        {
            bool recurse = true;

            OP_TYPE type = program.GetOpType(at);

            switch ( type )
            {
            case OP_TYPE::NU_CONDITIONAL:
            case OP_TYPE::SC_CONDITIONAL:
            case OP_TYPE::CO_CONDITIONAL:
            case OP_TYPE::IM_CONDITIONAL:
            case OP_TYPE::ME_CONDITIONAL:
            case OP_TYPE::LA_CONDITIONAL:
            case OP_TYPE::IN_CONDITIONAL:
			case OP_TYPE::ED_CONDITIONAL:
            {
				OP::ConditionalArgs args = program.GetOpArgs<OP::ConditionalArgs>(at);

                recurse = false;

                PARENT::RecurseWithCurrentState( args.condition );

                // If there is no expression, we'll assume true.
                bool value = true;

                if (args.condition)
                {
                    value = m_pSystem->BuildBool(m_pModel, m_pParams, args.condition);
                }

                if (value)
                {
                    PARENT::RecurseWithCurrentState( args.yes );
                }
                else
                {
                    PARENT::RecurseWithCurrentState( args.no );
                }
                break;
            }

            case OP_TYPE::NU_SWITCH:
            case OP_TYPE::SC_SWITCH:
            case OP_TYPE::CO_SWITCH:
            case OP_TYPE::IM_SWITCH:
            case OP_TYPE::ME_SWITCH:
            case OP_TYPE::LA_SWITCH:
            case OP_TYPE::IN_SWITCH:
			case OP_TYPE::ED_SWITCH:
            {
                recurse = false;

				const uint8* data = program.GetOpArgsPointer(at);
				
				OP::ADDRESS VarAddress;
				FMemory::Memcpy( &VarAddress, data, sizeof(OP::ADDRESS));
				data += sizeof(OP::ADDRESS);

                if (VarAddress)
                {
					OP::ADDRESS DefAddress;
					FMemory::Memcpy( &DefAddress, data, sizeof(OP::ADDRESS));
					data += sizeof(OP::ADDRESS);

					uint32 CaseCount;
					FMemory::Memcpy( &CaseCount, data, sizeof(uint32));
					data += sizeof(uint32);

                    PARENT::RecurseWithCurrentState( VarAddress );

                    int var = m_pSystem->BuildInt( m_pModel, m_pParams, VarAddress );

					OP::ADDRESS valueAt = DefAddress;
					for (uint32 C = 0; C < CaseCount; ++C)
					{
						int32 Condition;
						FMemory::Memcpy( &Condition, data, sizeof(int32));		
						data += sizeof(int32);

						OP::ADDRESS At;
						FMemory::Memcpy( &At, data, sizeof(OP::ADDRESS));
						data += sizeof(OP::ADDRESS);

						if (At && var == (int)Condition)
						{
							valueAt = At;
							break;	
						}
					}

					PARENT::RecurseWithCurrentState( valueAt );
                }

                break;
            }


            case OP_TYPE::IN_ADDLOD:
            {
                recurse = false;

				const uint8* Data = program.GetOpArgsPointer(at);

				uint8 LODCount;
				FMemory::Memcpy(&LODCount, Data, sizeof(uint8));
				Data += sizeof(uint8);

                STATE NewState = PARENT::GetCurrentState();
                for (int8 LODIndex=0; LODIndex < LODCount;++LODIndex)
                {
					OP::ADDRESS LODAddress;
					FMemory::Memcpy(&LODAddress, Data, sizeof(OP::ADDRESS));
					Data += sizeof(OP::ADDRESS);
					
					if (LODAddress)
                    {
                        bool bSelected = ( (1<< LODIndex) & m_lodMask ) != 0;
                        if (bSelected)
                        {
                            PARENT::RecurseWithState(LODAddress, NewState);
                        }
                    }
                }
                break;
            }


            case OP_TYPE::IN_ADDMESH:
            {
				OP::InstanceAddArgs args = program.GetOpArgs<OP::InstanceAddArgs>(at);

                recurse = false;

                PARENT::RecurseWithCurrentState(args.instance);

                STATE newState = PARENT::GetCurrentState();
                newState.m_underResourceCount=1;

                OP::ADDRESS meshAt = args.value;
                if (meshAt)
                {
                    PARENT::RecurseWithState(meshAt, newState);
                }
                break;
            }


            case OP_TYPE::IN_ADDIMAGE:
            {
				OP::InstanceAddArgs args = program.GetOpArgs<OP::InstanceAddArgs>(at);

                recurse = false;

                PARENT::RecurseWithCurrentState(args.instance);

                STATE newState = PARENT::GetCurrentState();
                newState.m_underResourceCount=1;

                OP::ADDRESS imageAt = args.value;
                if (imageAt)
                {
                    PARENT::RecurseWithState(imageAt, newState);
                }
                break;
            }

            default:
                break;
            }

            return recurse;
        }


    protected:
        System::Private* m_pSystem = nullptr;
		TSharedPtr<const Model> m_pModel;
        const Parameters* m_pParams = nullptr;
        unsigned m_lodMask = 0;
    };


    //---------------------------------------------------------------------------------------------
    //! Code visitor that:
    //! - only traverses the operations that are relevant for a given set of parameter values. It
    //! only considers the discrete parameters like integers and booleans. In the case of forks
    //! caused by continuous parameters like float weights for interpolation, all the branches are
    //! traversed.
    //! - cannot change the instructions
    //! - will not repeat visits to instructions with the same state
    //! - the state has to be a compatible with COVERED_CODE_VISITOR_STATE
    //---------------------------------------------------------------------------------------------
    template<typename COVERED_STATE=COVERED_CODE_VISITOR_STATE>
    class UniqueDiscreteCoveredCodeVisitor :
            public DiscreteCoveredCodeVisitorBase
            <
            UniqueConstCodeVisitorIterative<COVERED_STATE>,
            COVERED_STATE
            >
    {
        using PARENT=DiscreteCoveredCodeVisitorBase<UniqueConstCodeVisitorIterative<COVERED_STATE>, COVERED_STATE>;

    public:

        UniqueDiscreteCoveredCodeVisitor
            (
                System::Private* InSystem,
				const TSharedPtr<const Model>& InModel,
                const ParametersPtrConst& InParams,
                uint32 InLodMask
            )
            : PARENT(InSystem, InModel, InParams, InLodMask )
        {
        }

    };



    //---------------------------------------------------------------------------------------------
    //! Calculate all the parameters found under a particular operation
    //! It has an internal cache, so don't reuse objects of this class if the program changes.
    //---------------------------------------------------------------------------------------------
    class MUTABLERUNTIME_API SubtreeParametersVisitor
    {
    public:

        void Run( OP::ADDRESS Root, const FProgram& );

        //! After Run, list of relevant parameters.
		TArray<int32> RelevantParams;

    private:

		TArray<int32> CurrentParams;
		TArray<uint8> Visited;
		TArray<OP::ADDRESS> Pending;

        // Result cache
        TMap< OP::ADDRESS, TArray<int32> > ResultCache;
    };


}

