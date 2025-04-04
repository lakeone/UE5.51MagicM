// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

//#include "SlateBasics.h"

#include "Model/TextureGraphInsightSession.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
class STextureGraphInsightResourceViewRow; /// Declare the concrete type of widget used for the raws of the view. Defined in the cpp file
class ITableRow;
class STableViewBase;

class TEXTUREGRAPHINSIGHT_API STextureGraphInsightResourceView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STextureGraphInsightResourceView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	// TreeView Item Types
	class FItemData;
	using FItem = TSharedPtr<FItemData>;
	using FItemArray = TArray< FItem >;
	class FItemData
	{
	public:
		FItemData(RecordID rid) : _recordID(rid) {}
		TSharedPtr < STextureGraphInsightResourceViewRow > _widget;
		RecordID _recordID;
		FItemArray	_children;
	};
	//using SItemTableView = SListView< FItem >;
	using SItemTableView = STreeView< FItem >;


	// Standard delegates for the view
	TSharedRef<ITableRow>	OnGenerateRowForView(FItem item, const TSharedRef<STableViewBase>& OwnerTable);
	FORCEINLINE void		OnGetChildrenForView(FItem item, FItemArray& children) { children = item->_children; };
	void					OnClickItemForView(FItem item);
	void					OnDoubleClickItemForView(FItem item);

	/// The list of root items
	FItemArray _rootItems;

	// The TreeView widget
	TSharedPtr<SItemTableView> _tableView;

	void RefreshRootItems();

	void OnBlobNew(const RecordIDArray& rids);
	void OnBlobMapped(const RecordIDArray& rids);
	void OnEngineReset(int id);
};
