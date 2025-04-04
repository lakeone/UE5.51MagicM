// Copyright Epic Games, Inc. All Rights Reserved.

#include "SRewindDebugger.h"

#include "ActorPickerMode.h"
#include "Editor.h"
#include "IGameplayProvider.h"
#include "IRewindDebuggerTrackCreator.h"
#include "LocalizationDescriptor.h"
#include "Editor/EditorEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Insights/IUnrealInsightsModule.h"
#include "Modules/ModuleManager.h"
#include "ObjectTrace.h"
#include "RewindDebuggerStyle.h"
#include "RewindDebuggerCommands.h"
#include "SSimpleTimeSlider.h"
#include "SceneOutlinerPublicTypes.h"
#include "SceneOutlinerModule.h"
#include "Selection.h"
#include "Styling/SlateIconFinder.h"
#include "ToolMenu.h"
#include "ToolMenus.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Kismet2/DebuggerCommands.h"
#include "RewindDebuggerSettings.h"
#include "Widgets/Input/STextComboBox.h"

#define LOCTEXT_NAMESPACE "SRewindDebugger"

SRewindDebugger::SRewindDebugger() 
	: SCompoundWidget()
	, ViewRange(0,10)
	, Commands(FRewindDebuggerCommands::Get())
	, DebugComponents(nullptr)
	, Settings(URewindDebuggerSettings::Get())
{ 
}

SRewindDebugger::~SRewindDebugger() 
{
}

void SRewindDebugger::TrackCursor(bool bReverse)
{
	float ScrubTime = ScrubTimeAttribute.Get();
	TRange<double> CurrentViewRange = ViewRange;
	float ViewSize = CurrentViewRange.GetUpperBoundValue() - CurrentViewRange.GetLowerBoundValue();

	static const double LeadingEdgeSize = 0.05;
	static const double TrailingEdgeThreshold = 0.01;

	if(bReverse)
	{
		// playing in reverse (cursor moving left)
		if (ScrubTime < CurrentViewRange.GetLowerBoundValue() + ViewSize * LeadingEdgeSize)
		{
			CurrentViewRange.SetLowerBound(ScrubTime - ViewSize * LeadingEdgeSize);
			CurrentViewRange.SetUpperBound(CurrentViewRange.GetLowerBoundValue() + ViewSize);
		}
		if (ScrubTime > CurrentViewRange.GetUpperBoundValue() + ViewSize * TrailingEdgeThreshold)
		{
			CurrentViewRange.SetUpperBound(ScrubTime);
			CurrentViewRange.SetLowerBound(CurrentViewRange.GetUpperBoundValue() - ViewSize);
		}
	}
	else
	{
		// playing normally or recording (cursor moving right)
		if (ScrubTime > CurrentViewRange.GetUpperBoundValue() - ViewSize * LeadingEdgeSize)
		{
			CurrentViewRange.SetUpperBound(ScrubTime + ViewSize * LeadingEdgeSize);
			CurrentViewRange.SetLowerBound(CurrentViewRange.GetUpperBoundValue() - ViewSize);
		}
		if (ScrubTime < CurrentViewRange.GetLowerBoundValue() - ViewSize * TrailingEdgeThreshold)
		{
			CurrentViewRange.SetLowerBound(ScrubTime);
			CurrentViewRange.SetUpperBound(CurrentViewRange.GetLowerBoundValue() + ViewSize);
		}
	}

	SetViewRange(CurrentViewRange);
}

void SRewindDebugger::SetViewRange(TRange<double> NewRange)
{
	ViewRange = NewRange;
	OnViewRangeChanged.ExecuteIfBound(NewRange);
}

void SRewindDebugger::ToggleHideTrackType(const FName& TrackType)
{
	int32 Index = Settings.HiddenTrackTypes.Find(TrackType);

	if (Index >=0)
	{
		Settings.HiddenTrackTypes.RemoveAtSwap(Index);
	}
	else
	{
		Settings.HiddenTrackTypes.Add(TrackType);
	}
	RefreshDebugComponents();
}

bool SRewindDebugger::ShouldHideTrackType(const FName& TrackType) const
{
	return Settings.HiddenTrackTypes.Contains(TrackType);
}


void SRewindDebugger::ToggleDisplayEmptyTracks()
{
	Settings.bShowEmptyObjectTracks = !Settings.bShowEmptyObjectTracks;
	RefreshDebugComponents();
}

bool SRewindDebugger::ShouldDisplayEmptyTracks() const
{
	return Settings.bShowEmptyObjectTracks;
}

FReply SRewindDebugger::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FInputChord KeyEventAsInputChord = FInputChord(InKeyEvent.GetKey(), EModifierKey::FromBools(InKeyEvent.IsControlDown(), InKeyEvent.IsAltDown(), InKeyEvent.IsShiftDown(), InKeyEvent.IsCommandDown()));
	FReply bReply = FReply::Unhandled();

	// Handle Rewind Debugger VCR Commands
	if (CommandList->ProcessCommandBindings(InKeyEvent))
	{
		bReply = FReply::Handled();
	}

	// Prevent bubbling up shortcut for "ReversePlay"
	if (Commands.ReversePlay->HasDefaultChord(KeyEventAsInputChord) && !IsPIESimulating.Get())
	{
		bReply = FReply::Handled();
	}

	return bReply;
}

void SRewindDebugger::SetDebugTargetActor(AActor* Actor)
{
	FString ActorLabel = Actor->GetActorLabel();
	// Spawned actors have a "RewindDebugger: " prefix on their label
	if (ActorLabel.StartsWith("RewindDebugger: "))
	{
		ActorLabel.RemoveFromStart("RewindDebugger: ");
		DebugTargetActor.Set(ActorLabel);
	}
	else
	{
		DebugTargetActor.Set(Actor->GetName());
	}
}

TSharedRef<SWidget> SRewindDebugger::MakeSelectActorMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddSearchWidget();

	// Menu entry for each actor selected in the scene
	TArray<AActor*> SelectedActors;
	GEditor->GetSelectedActors()->GetSelectedObjects(SelectedActors);

	MenuBuilder.BeginSection("From Selection Section", LOCTEXT("FromSelection", "From Scene Selection"));
	if (SelectedActors.Num() > 0)
	{
		for(AActor* SelectedActor : SelectedActors)
		{
			FText SelectedLabel = FText::FromString(SelectedActor->GetActorLabel());
			FSlateIcon ActorIcon = FSlateIconFinder::FindIconForClass(SelectedActors[0]->GetClass());

			MenuBuilder.AddMenuEntry(SelectedLabel, FText(), ActorIcon, FExecuteAction::CreateLambda([this, SelectedActor]{
				FSlateApplication::Get().DismissAllMenus();
				SetDebugTargetActor(SelectedActor);
			}));
		}
	}
	else
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("No scene selection", "No scene selection"),
		 LOCTEXT("SceneSelectionToolTip", "If you select an object in the scene, then it will be listed here"),
		 FSlateIcon(),
		  FUIAction(FExecuteAction(),
		  	FCanExecuteAction::CreateLambda([](){return false;})));
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("From Recording Section", LOCTEXT("FromRecording", "From Recording:"));

	if (IRewindDebugger* RewindDebugger = IRewindDebugger::Instance())
	{
		if (const TraceServices::IAnalysisSession* Session = RewindDebugger->GetAnalysisSession())
		{
			TraceServices::FAnalysisSessionReadScope SessionReadScope(*Session);
			const IGameplayProvider* GameplayProvider = Session->ReadProvider<IGameplayProvider>("GameplayProvider");

			const FClassInfo* ActorClassInfo = GameplayProvider->FindClassInfo(*AActor::StaticClass()->GetPathName());
			
			GameplayProvider->EnumerateObjects([this, &MenuBuilder, GameplayProvider, ActorClassInfo](const FObjectInfo& ObjectInfo)
			{
				if (GameplayProvider->IsSubClassOf(ObjectInfo.ClassId, ActorClassInfo->Id))
				{
					FString ActorName = ObjectInfo.Name;
					FText SelectedLabel = FText::FromString(ActorName);
					FSlateIcon ActorIcon = GameplayProvider->FindIconForClass(ObjectInfo.ClassId);
					const FClassInfo& ClassInfo = GameplayProvider->GetClassInfo(ObjectInfo.ClassId);

					MenuBuilder.AddMenuEntry(SelectedLabel, FText(), ActorIcon, FExecuteAction::CreateLambda([this, ActorName]()
					{
						FSlateApplication::Get().DismissAllMenus();
						DebugTargetActor.Set(ActorName);
					}));
				}
			});
		}
		else
		{
			MenuBuilder.AddMenuEntry(LOCTEXT("No recording loaded", "No recording loaded"),
			 LOCTEXT("NoRecordingToolTip", "Start or load a recording, and the recorded actors will be listed here"),
			 FSlateIcon(),
			  FUIAction(FExecuteAction(),
				FCanExecuteAction::CreateLambda([](){return false;})));
        }
		
	}

	return MenuBuilder.MakeWidget();
}

void SRewindDebugger::Construct(const FArguments& InArgs, TSharedRef<FUICommandList> InCommandList, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	OnScrubPositionChanged = InArgs._OnScrubPositionChanged;
	OnViewRangeChanged = InArgs._OnViewRangeChanged;
	OnComponentSelectionChanged = InArgs._OnComponentSelectionChanged;
	BuildComponentContextMenu = InArgs._BuildComponentContextMenu;
	TrackTypesAttribute = InArgs._TrackTypes;
	ScrubTimeAttribute = InArgs._ScrubTime;
	DebugComponents = InArgs._DebugComponents;
	TraceTime.Initialize(InArgs._TraceTime);
	RecordingDuration.Initialize(InArgs._RecordingDuration);
	DebugTargetActor.Initialize(InArgs._DebugTargetActor);
	IsPIESimulating = InArgs._IsPIESimulating;
	CommandList = InCommandList;

	TrackFilterBox = SNew(SSearchBox).HintText(LOCTEXT("Filter Tracks","Filter Tracks")).OnTextChanged_Lambda([this](const FText&)
	{
		RefreshDebugComponents();
	});
	
	TSharedPtr<SScrollBar> ScrollBar = SNew(SScrollBar);
	FToolMenuContext ToolMenuContext(CommandList);

	ComponentTreeView =	SNew(SRewindDebuggerComponentTree)
		.ExternalScrollBar(ScrollBar)
		.OnExpansionChanged_Lambda([this]()
		{
			if (!bInExpansionChanged)
			{
				bInExpansionChanged = true;
				TimelinesView->RestoreExpansion();
				bInExpansionChanged = false;
			}
		})
		.OnScrolled_Lambda([this](double ScrollOffset){ TimelinesView->ScrollTo(ScrollOffset); })
		.DebugComponents(InArgs._DebugComponents)
		.OnMouseButtonDoubleClick(InArgs._OnComponentDoubleClicked)
		.OnContextMenuOpening(this, &SRewindDebugger::OnContextMenuOpening)
		.OnSelectionChanged_Lambda(
			[this](TSharedPtr<RewindDebugger::FRewindDebuggerTrack> SelectedItem, ESelectInfo::Type SelectInfo)
			{
				if (!bInSelectionChanged)
				{
					bInSelectionChanged = true;
					TimelinesView->SetSelection(SelectedItem);
					ComponentSelectionChanged(SelectedItem, SelectInfo);
					bInSelectionChanged = false;
				}
			});

	 TimelinesView = SNew(SRewindDebuggerTimelines)
		.ExternalScrollbar(ScrollBar)
		.OnExpansionChanged_Lambda(
				[this]()
				{
					if (!bInExpansionChanged)
					{
						bInExpansionChanged = true;
						ComponentTreeView->RestoreExpansion();
						bInExpansionChanged = false;
					}
				})
		.OnScrolled_Lambda([this](double ScrollOffset){ ComponentTreeView->ScrollTo(ScrollOffset); })
		.DebugComponents(InArgs._DebugComponents)
		.ViewRange_Lambda([this](){return ViewRange;})
		.ClampRange_Lambda(
			 [this]()
			 {
				 return TRange<double>(0.0f, RecordingDuration.Get());
			 })
		.OnViewRangeChanged(this, &SRewindDebugger::SetViewRange)
		.ScrubPosition(ScrubTimeAttribute)
		.OnScrubPositionChanged_Lambda(
			[this](double NewScrubTime, bool bIsScrubbing)
			{
				if (bIsScrubbing)
				{
					OnScrubPositionChanged.ExecuteIfBound( NewScrubTime, bIsScrubbing );
				}
			})
		.OnSelectionChanged_Lambda(
			[this](TSharedPtr<RewindDebugger::FRewindDebuggerTrack> SelectedItem, ESelectInfo::Type SelectInfo)
			{
				if (!bInSelectionChanged)
				{
					bInSelectionChanged = true;
					ComponentTreeView->SetSelection(SelectedItem);
					ComponentSelectionChanged(SelectedItem, SelectInfo);
					bInSelectionChanged = false;
				}
			});

	
	TSharedRef<SWidget> ToolBar = UToolMenus::Get()->GenerateWidget("RewindDebugger.ToolBar", ToolMenuContext);

	ChildSlot
	[
		SNew(SVerticalBox)
		 + SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().MaxHeight(48)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SComboButton)
						.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
						.OnGetMenuContent(this, &SRewindDebugger::MakeMainMenu)
						.ButtonContent()
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("ClassIcon.CameraComponent"))
					]
				]
				+SHorizontalBox::Slot().FillWidth(1.0)
				[
					ToolBar
				]
			]
		]
		 + SVerticalBox::Slot().FillHeight(1.0)
		 [
			SNew(SSplitter)
			+SSplitter::Slot().MinSize(280).Value(0)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot() .FillWidth(1.0f)
					[
						// SNew(SRewindDebuggerTargetSelector)
						SNew(SComboButton)
						.OnGetMenuContent(this, &SRewindDebugger::MakeSelectActorMenu)
						.ButtonContent()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot().AutoWidth().Padding(3)
							[
								SNew(SImage)
								.Image_Lambda([this]
									{
										if (DebugComponents != nullptr && DebugComponents->Num()>0)
										{
											return (*DebugComponents)[0]->GetIcon().GetIcon();
										}

										return FSlateIconFinder::FindIconForClass(AActor::StaticClass()).GetIcon();
									}
								)
							]
							+SHorizontalBox::Slot().Padding(3)
							[
								SNew(STextBlock)
								.Text_Lambda([this](){
									if (DebugComponents == nullptr || DebugComponents->Num()==0)
									{
										return LOCTEXT("Select Actor", "Debug Target Actor");
									}

									FText ReadableName = (*DebugComponents)[0]->GetDisplayName();
#if OBJECT_TRACE_ENABLED
									if (UObject* Object = FObjectTrace::GetObjectFromId((*DebugComponents)[0]->GetObjectId()))
									{
										if (AActor* Actor = Cast<AActor>(Object))
										{
											ReadableName = FText::FromString(Actor->GetActorLabel());
										}
									}
#endif // OBJECT_TRACE_ENABLED

									return ReadableName;
								} )
							]
						]
					]
					+SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)
					[
						SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.OnClicked(this, &SRewindDebugger::OnSelectActorClicked)
							.ToolTipText(LOCTEXT("SelectActorTooltip", "Select Target Actor in Scene"))
						[
							SNew(SImage)
							.Image(FRewindDebuggerStyle::Get().GetBrush("RewindDebugger.SelectActor"))
						]
					]
					+SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SComboButton)
							.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
							.OnGetMenuContent(this, &SRewindDebugger::MakeFilterMenu)
							.ButtonContent()
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush("Icons.Filter"))
						]
					]
				]
				+SVerticalBox::Slot().FillHeight(1.0f)
				[
					ComponentTreeView.ToSharedRef()
				]
			]
			+SSplitter::Slot() 
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(SSimpleTimeSlider)
						.DesiredSize({100,24})
						.ClampRangeHighlightSize(0.15f)
						.ClampRangeHighlightColor(FLinearColor::Red.CopyWithNewOpacity(0.5f))
						.ScrubPosition(ScrubTimeAttribute)
						.ViewRange_Lambda([this](){ return ViewRange; })
						.OnViewRangeChanged(this, &SRewindDebugger::SetViewRange)
						.ClampRange_Lambda(
								[this]()
								{ 
									return TRange<double>(0.0f,RecordingDuration.Get());
								})	
						.OnScrubPositionChanged_Lambda(
							[this](double NewScrubTime, bool bIsScrubbing)
									{
										if (bIsScrubbing)
										{
											OnScrubPositionChanged.ExecuteIfBound( NewScrubTime, bIsScrubbing );
										}
									})
				]
				+SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[				
						TimelinesView.ToSharedRef()
					]
					+SOverlay::Slot().HAlign(EHorizontalAlignment::HAlign_Right)
					[
						ScrollBar.ToSharedRef()
					]
					
				]
			]
		]
	];
}

bool FilterTrack(TSharedPtr<RewindDebugger::FRewindDebuggerTrack>& Track, const FString& FilterString, bool bRemoveNoData, const TArray<FName>& FilteredTrackTypes, bool bParentFilterPassed = false)
{
	if (FilteredTrackTypes.Contains(Track->GetName()))
	{
		Track->SetIsVisible(false);
		return false;
	}
	
	const bool bStringFilterEmpty =  FilterString.IsEmpty();
	const bool bStringFilterPassed = bParentFilterPassed || bStringFilterEmpty || Track->GetDisplayName().ToString().Contains(FilterString);

	const bool bThisFilterPassed = (!bStringFilterEmpty && bStringFilterPassed);


	bool bAnyChildVisible = false;
	Track->IterateSubTracks([&bAnyChildVisible, bThisFilterPassed, FilterString, bRemoveNoData, FilteredTrackTypes](TSharedPtr<RewindDebugger::FRewindDebuggerTrack> ChildTrack)
	{
		const bool bChildIsVisible = FilterTrack(ChildTrack, FilterString, bRemoveNoData, FilteredTrackTypes, bThisFilterPassed);
		bAnyChildVisible |= bChildIsVisible;
	});

	bool bVisible = bAnyChildVisible || ((!bRemoveNoData || Track->HasDebugData()) && bStringFilterPassed);

	Track->SetIsVisible(bVisible);
	return bVisible;
}

void SRewindDebugger::RefreshDebugComponents()
{
	if (DebugComponents)
	{
		for(TSharedPtr<RewindDebugger::FRewindDebuggerTrack>& DebugComponent : *DebugComponents)
		{
			FilterTrack(DebugComponent, TrackFilterBox->GetText().ToString(), !ShouldDisplayEmptyTracks(), Settings.HiddenTrackTypes);
		}
	}
	
	ComponentTreeView->Refresh();
	TimelinesView->Refresh();
}

TSharedRef<SWidget> SRewindDebugger::MakeMainMenu()
{
	return UToolMenus::Get()->GenerateWidget("RewindDebugger.MainMenu", FToolMenuContext());
}

TSharedRef<SWidget> SRewindDebugger::MakeFilterMenu()
{
	FMenuBuilder Builder(true, nullptr);
	Builder.AddWidget(TrackFilterBox.ToSharedRef(), FText(), true, false);
	
	Builder.BeginSection("TrackTypes", LOCTEXT("Track Types", "Track Types"));

	const TArrayView<RewindDebugger::FRewindDebuggerTrackType> TrackTypes = TrackTypesAttribute.Get();

	for(const RewindDebugger::FRewindDebuggerTrackType& TrackType : TrackTypes)
	{
		Builder.AddMenuEntry(
			TrackType.DisplayName,
			FText::Format(LOCTEXT("FilterTrackToolTip", "Show tracks of type {0}"), {TrackType.DisplayName}),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this,TrackType](){ ToggleHideTrackType(TrackType.Name); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this,TrackType]() { return !ShouldHideTrackType(TrackType.Name); })),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);	
	}

	Builder.AddSeparator();

	Builder.AddMenuEntry(
		LOCTEXT("DisplayEmptyTracks", "Show Empty Object Tracks"),
		LOCTEXT("DisplayEmptyTracksToolTip", "Show Object/Component tracks which have no sub tracks with any debug data"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &SRewindDebugger::ToggleDisplayEmptyTracks),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &SRewindDebugger::ShouldDisplayEmptyTracks)),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	
	return Builder.MakeWidget();
}

void SRewindDebugger::ComponentSelectionChanged(TSharedPtr<RewindDebugger::FRewindDebuggerTrack> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedComponent)
	{
		SelectedComponent->SetIsSelected(false);
	}
	
	SelectedComponent = SelectedItem;
	
	if (SelectedComponent)
	{
		SelectedComponent->SetIsSelected(true);
	}
	

	OnComponentSelectionChanged.ExecuteIfBound(SelectedItem);
}

TSharedPtr<SWidget> SRewindDebugger::OnContextMenuOpening()
{
	return BuildComponentContextMenu.Execute();
}

FReply SRewindDebugger::OnSelectActorClicked()
{
	FActorPickerModeModule& ActorPickerMode = FModuleManager::Get().GetModuleChecked<FActorPickerModeModule>("ActorPickerMode");
	
	const bool bShouldForceEject = GEditor->PlayWorld && !GEditor->bIsSimulatingInEditor;
	if (bShouldForceEject)
	{
		// Eject PIE
		GEditor->RequestToggleBetweenPIEandSIE();
	}

	ActorPickerMode.BeginActorPickingMode(
		FOnGetAllowedClasses(), 
		FOnShouldFilterActor(),
		FOnActorSelected::CreateLambda([this, bShouldForceEject](AActor* InActor)
		{
			SetDebugTargetActor(InActor);
			if (bShouldForceEject && GEditor->bIsSimulatingInEditor)
			{
				// If we force ejected PIE, revert this after actor selection.
				GEditor->RequestToggleBetweenPIEandSIE();
			}
		}));



	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
