#include "DSAIEditorPrivatePCH.h"
#include "AnimGraphNode_DynamicSubInstance.h"

#include "CompilerResultsLog.h"
#include "PropertyEditing.h"
#include "IPropertyUtilities.h"
#include "Animation/AnimNode_SubInput.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Animation/AnimInstance.h"

#define LOCTEXT_NAMESPACE "DynamicSubInstanceNode"

namespace SubInstanceGraphNodeConstants
{
	FLinearColor TitleColor(0.2f, 0.2f, 0.8f);
}

FLinearColor UAnimGraphNode_DynamicSubInstance::GetNodeTitleColor() const
{
	return SubInstanceGraphNodeConstants::TitleColor;
}

FText UAnimGraphNode_DynamicSubInstance::GetTooltipText() const
{
	return LOCTEXT("ToolTip", "Runs a sub-anim instance to process animation.");
}
FText UAnimGraphNode_DynamicSubInstance::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Sub Anim Instance (Dynamic)");
}

void UAnimGraphNode_DynamicSubInstance::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	UAnimBlueprint* AnimBP = CastChecked<UAnimBlueprint>(GetBlueprint());

	UObject* OriginalNode = MessageLog.FindSourceObject(this);

	/* Check we have a class set
	if (!*Node.InputClass)
	{
		MessageLog.Error(TEXT("Sub instance node @@ has no valid instance class to spawn."), this);
	}*/

	// Check for cycles from other sub instance nodes
	TArray<UEdGraph*> Graphs;
	AnimBP->GetAllGraphs(Graphs);

	for (UEdGraph* Graph : Graphs)
	{
		TArray<UAnimGraphNode_DynamicSubInstance*> SubInstanceNodes;
		Graph->GetNodesOfClass(SubInstanceNodes);

		for (UAnimGraphNode_DynamicSubInstance* SubInstanceNode : SubInstanceNodes)
		{
			if (SubInstanceNode == OriginalNode)
			{
				continue;
			}

			FAnimNode_DynamicSubInstance& InnerNode = SubInstanceNode->Node;

			if (*InnerNode.InputClass && *InnerNode.InputClass == *Node.InputClass)
			{
				MessageLog.Error(TEXT("Node @@ and node @@ both target the same class @@, causing a sub instance loop."), this, SubInstanceNode, *Node.InputClass);
			}
		}
	}

	if (HasInstanceLoop())
	{
		MessageLog.Error(TEXT("Detected loop in sub instance chain starting at @@ inside class @@"), this, AnimBP->GetAnimBlueprintGeneratedClass());
	}

	// Check we don't try to spawn our own blueprint
	if (*Node.InputClass == AnimBP->GetAnimBlueprintGeneratedClass())
	{
		MessageLog.Error(TEXT("Sub instance node @@ targets instance class @@ which it is inside, this would cause a loop."), this, AnimBP->GetAnimBlueprintGeneratedClass());
	}
}

void UAnimGraphNode_DynamicSubInstance::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	UClass* TargetClass = *Node.InputClass;

	if (!TargetClass)
	{
		// Nothing to search for properties
		return;
	}

	// Need the schema to extract pin types
	const UEdGraphSchema_K2* Schema = CastChecked<UEdGraphSchema_K2>(GetSchema());

	bool bShowPose = false;

	// Scan the target class for a sub input node, we only want to show the pose input if
	// we have that node available
	for (TFieldIterator<UProperty> It(TargetClass); It; ++It)
	{
		UProperty* CurrentProp = *It;

		if (UStructProperty* StructProp = Cast<UStructProperty>(CurrentProp))
		{
			if (StructProp->Struct->IsChildOf(FAnimNode_SubInput::StaticStruct()))
			{
				// Found a pose input
				bShowPose = true;
				break;
			}
		}
	}

	if (bShowPose)
	{
		if (UProperty* PoseProperty = FindField<UProperty>(FAnimNode_DynamicSubInstance::StaticStruct(), GET_MEMBER_NAME_CHECKED(FAnimNode_DynamicSubInstance, InPose)))
		{
			FEdGraphPinType PinType;
			if (Schema->ConvertPropertyToPinType(PoseProperty, PinType))
			{
				UEdGraphPin* NewPin = CreatePin(EEdGraphPinDirection::EGPD_Input, PinType, PoseProperty->GetName());
				NewPin->PinFriendlyName = PoseProperty->GetDisplayNameText();

				CustomizePinData(NewPin, PoseProperty->GetFName(), INDEX_NONE);
			}
		}
	}

	// Grab the list of properties we can expose
	TArray<UProperty*> ExposablePropeties;
	GetExposableProperties(ExposablePropeties);

	// We'll track the names we encounter by removing from this list, if anything remains the properties
	// have been removed from the target class and we should remove them too
	TArray<FName> BeginExposableNames = KnownExposableProperties;

	for (UProperty* Property : ExposablePropeties)
	{
		FName PropertyName = Property->GetFName();
		BeginExposableNames.Remove(PropertyName);

		if (!KnownExposableProperties.Contains(PropertyName))
		{
			// New property added to the target class
			KnownExposableProperties.Add(PropertyName);
		}

		if (ExposedPropertyNames.Contains(PropertyName))
		{
			FEdGraphPinType PinType;

			verify(Schema->ConvertPropertyToPinType(Property, PinType));

			UEdGraphPin* NewPin = CreatePin(EEdGraphPinDirection::EGPD_Input, PinType, Property->GetName());
			NewPin->PinFriendlyName = Property->GetDisplayNameText();

			// Need to grab the default value for the property from the target generated class CDO
			FString CDODefaultValueString;
			uint8* ContainerPtr = reinterpret_cast<uint8*>(TargetClass->GetDefaultObject());

			if (FBlueprintEditorUtils::PropertyValueToString(Property, ContainerPtr, CDODefaultValueString))
			{
				// If we successfully pulled a value, set it to the pin
				Schema->TrySetDefaultValue(*NewPin, CDODefaultValueString);
			}

			CustomizePinData(NewPin, PropertyName, INDEX_NONE);
		}
	}

	// Remove any properties that no longer exist on the target class
	for (FName& RemovedPropertyName : BeginExposableNames)
	{
		KnownExposableProperties.Remove(RemovedPropertyName);
		ExposedPropertyNames.Remove(RemovedPropertyName);
	}

}

void UAnimGraphNode_DynamicSubInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bool bRequiresNodeReconstruct = false;
	UProperty* ChangedProperty = PropertyChangedEvent.Property;

	if (ChangedProperty)
	{
		if (ChangedProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FAnimNode_DynamicSubInstance, InputClass))
		{
			bRequiresNodeReconstruct = true;
			RebuildExposedProperties(*Node.InputClass);
		}
	}

	if (bRequiresNodeReconstruct)
	{
		ReconstructNode();
	}
}

void UAnimGraphNode_DynamicSubInstance::GetInstancePinProperty(const UClass* InOwnerInputClass, UEdGraphPin* InInputPin, UProperty*& OutProperty)
{
	// The actual name of the instance property
	FString FullName = GetPinTargetVariableName(InInputPin);

	if (UProperty* Property = FindField<UProperty>(InOwnerInputClass, *FullName))
	{
		OutProperty = Property;
	}
	else
	{
		OutProperty = nullptr;
	}
}

FString UAnimGraphNode_DynamicSubInstance::GetPinTargetVariableName(const UEdGraphPin* InPin) const
{
	return TEXT("__SUBINSTANCE_") + InPin->PinName + TEXT("_") + NodeGuid.ToString();
}

void UAnimGraphNode_DynamicSubInstance::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	Super::CustomizeDetails(DetailBuilder);

	TArray<UProperty*> ExposableProperties;
	GetExposableProperties(ExposableProperties);

	if (ExposableProperties.Num() > 0)
	{
		IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(FName(TEXT("Sub Instance Properties")));

		for (UProperty* Property : ExposableProperties)
		{
			FDetailWidgetRow& WidgetRow = CategoryBuilder.AddCustomRow(FText::FromString(Property->GetName()));

			FName PropertyName = Property->GetFName();
			FText PropertyTypeText = GetPropertyTypeText(Property);

			FFormatNamedArguments Args;
			Args.Add(TEXT("PropertyName"), FText::FromName(PropertyName));
			Args.Add(TEXT("PropertyType"), PropertyTypeText);

			FText TooltipText = FText::Format(LOCTEXT("PropertyTooltipText", "{PropertyName}\nType: {PropertyType}"), Args);

			WidgetRow.NameContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Property->GetName()))
				.ToolTipText(TooltipText)
				];

			WidgetRow.ValueContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ExposePropertyValue", "Expose: "))
				]
			+ SHorizontalBox::Slot()
				[
					SNew(SCheckBox)
					.IsChecked_UObject(this, &UAnimGraphNode_DynamicSubInstance::IsPropertyExposed, PropertyName)
				.OnCheckStateChanged_UObject(this, &UAnimGraphNode_DynamicSubInstance::OnPropertyExposeCheckboxChanged, PropertyName)
				]
				];
		}
	}

	TSharedRef<IPropertyHandle> ClassHandle = DetailBuilder.GetProperty(TEXT("Node.InputClass"), GetClass());
	if (ClassHandle->IsValidHandle())
	{
		ClassHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateUObject(this, &UAnimGraphNode_DynamicSubInstance::OnInputClassChanged, &DetailBuilder));
	}

	ClassHandle->MarkHiddenByCustomization();

	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(FName(TEXT("Settings")));

	FDetailWidgetRow& ClassWidgetRow = CategoryBuilder.AddCustomRow(LOCTEXT("FilterString", "Instance Class"));
	ClassWidgetRow.NameContent()
		[
			ClassHandle->CreatePropertyNameWidget()
		];

	ClassWidgetRow.ValueContent()
		[
			SNew(SObjectPropertyEntryBox)
			.ObjectPath_UObject(this, &UAnimGraphNode_DynamicSubInstance::GetCurrentInstanceBlueprintPath)
		.AllowedClass(UAnimBlueprint::StaticClass())
		.NewAssetFactories(TArray<UFactory*>())
		.OnShouldFilterAsset(FOnShouldFilterAsset::CreateUObject(this, &UAnimGraphNode_DynamicSubInstance::OnShouldFilterInstanceBlueprint))
		.OnObjectChanged(FOnSetObject::CreateUObject(this, &UAnimGraphNode_DynamicSubInstance::OnSetInstanceBlueprint, ClassHandle))
		];
}

FText UAnimGraphNode_DynamicSubInstance::GetPropertyTypeText(UProperty* Property)
{
	FText PropertyTypeText;

	if (UStructProperty* StructProperty = Cast<UStructProperty>(Property))
	{
		PropertyTypeText = StructProperty->Struct->GetDisplayNameText();
	}
	else if (UObjectProperty* ObjectProperty = Cast<UObjectProperty>(Property))
	{
		PropertyTypeText = ObjectProperty->PropertyClass->GetDisplayNameText();
	}
	else if (UClass* PropClass = Property->GetClass())
	{
		PropertyTypeText = PropClass->GetDisplayNameText();
	}
	else
	{
		PropertyTypeText = LOCTEXT("PropertyTypeUnknown", "Unknown");
	}

	return PropertyTypeText;
}

void UAnimGraphNode_DynamicSubInstance::RebuildExposedProperties(UClass* InNewClass)
{
	ExposedPropertyNames.Empty();
	KnownExposableProperties.Empty();
	if (InNewClass)
	{
		TArray<UProperty*> ExposableProperties;
		GetExposableProperties(ExposableProperties);

		for (UProperty* Property : ExposableProperties)
		{
			KnownExposableProperties.Add(Property->GetFName());
		}
	}
}

bool UAnimGraphNode_DynamicSubInstance::HasInstanceLoop()
{
	TArray<FGuid> VisitedList;
	TArray<FGuid> CurrentStack;
	return HasInstanceLoop_Recursive(this, VisitedList, CurrentStack);
}

bool UAnimGraphNode_DynamicSubInstance::HasInstanceLoop_Recursive(UAnimGraphNode_DynamicSubInstance* CurrNode, TArray<FGuid>& VisitedNodes, TArray<FGuid>& NodeStack)
{
	if (!VisitedNodes.Contains(CurrNode->NodeGuid))
	{
		VisitedNodes.Add(CurrNode->NodeGuid);
		NodeStack.Add(CurrNode->NodeGuid);

		if (UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(UBlueprint::GetBlueprintFromClass(CurrNode->Node.InputClass)))
		{
			// Check for cycles from other sub instance nodes
			TArray<UEdGraph*> Graphs;
			AnimBP->GetAllGraphs(Graphs);

			for (UEdGraph* Graph : Graphs)
			{
				TArray<UAnimGraphNode_DynamicSubInstance*> SubInstanceNodes;
				Graph->GetNodesOfClass(SubInstanceNodes);

				for (UAnimGraphNode_DynamicSubInstance* SubInstanceNode : SubInstanceNodes)
				{
					// If we haven't visited this node, then check it for loops, otherwise if we're pointing to a previously visited node that is in the current instance stack we have a loop
					if ((!VisitedNodes.Contains(SubInstanceNode->NodeGuid) && HasInstanceLoop_Recursive(SubInstanceNode, VisitedNodes, NodeStack)) || NodeStack.Contains(SubInstanceNode->NodeGuid))
					{
						return true;
					}
				}
			}
		}
	}

	NodeStack.Remove(CurrNode->NodeGuid);
	return false;
}

ECheckBoxState UAnimGraphNode_DynamicSubInstance::IsPropertyExposed(FName PropertyName) const
{
	return ExposedPropertyNames.Contains(PropertyName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void UAnimGraphNode_DynamicSubInstance::OnPropertyExposeCheckboxChanged(ECheckBoxState NewState, FName PropertyName)
{
	if (NewState == ECheckBoxState::Checked)
	{
		ExposedPropertyNames.AddUnique(PropertyName);
	}
	else if (NewState == ECheckBoxState::Unchecked)
	{
		ExposedPropertyNames.Remove(PropertyName);
	}

	ReconstructNode();
}

void UAnimGraphNode_DynamicSubInstance::OnInputClassChanged(IDetailLayoutBuilder* DetailBuilder)
{
	if (DetailBuilder)
	{
		DetailBuilder->ForceRefreshDetails();
	}
}

FString UAnimGraphNode_DynamicSubInstance::GetCurrentInstanceBlueprintPath() const
{
	UClass* InputClass = *Node.InputClass;

	if (InputClass)
	{
		UBlueprint* ActualBlueprint = UBlueprint::GetBlueprintFromClass(InputClass);

		if (ActualBlueprint)
		{
			return ActualBlueprint->GetPathName();
		}
	}

	return FString();
}

bool UAnimGraphNode_DynamicSubInstance::OnShouldFilterInstanceBlueprint(const FAssetData& AssetData) const
{
	if (const FString* SkeletonName = AssetData.TagsAndValues.Find(TEXT("TargetSkeleton")))
	{
		if (UAnimBlueprint* CurrentBlueprint = Cast<UAnimBlueprint>(GetBlueprint()))
		{
			FString TargetSkeletonName = FString::Printf(TEXT("%s'%s'"), *CurrentBlueprint->TargetSkeleton->GetClass()->GetName(), *CurrentBlueprint->TargetSkeleton->GetPathName());

			return *SkeletonName != TargetSkeletonName;
		}
	}

	return false;
}

void UAnimGraphNode_DynamicSubInstance::OnSetInstanceBlueprint(const FAssetData& AssetData, TSharedRef<IPropertyHandle> InputClassPropHandle)
{
	if (UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(AssetData.GetAsset()))
	{
		FScopedTransaction Transaction(LOCTEXT("SetBP", "Set Instance Blueprint"));

		Modify();

		InputClassPropHandle->SetValue(Blueprint->GetAnimBlueprintGeneratedClass());
	}
}

UObject* UAnimGraphNode_DynamicSubInstance::GetJumpTargetForDoubleClick() const
{
	UClass* InputClass = *Node.InputClass;

	if (InputClass)
	{
		return InputClass->ClassGeneratedBy;
	}

	return nullptr;
}

bool UAnimGraphNode_DynamicSubInstance::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput /*= NULL*/) const
{
	UClass* InputClassToUse = *Node.InputClass;

	// Add our instance class... If that changes we need a recompile
	if (InputClassToUse && OptionalOutput)
	{
		OptionalOutput->AddUnique(InputClassToUse);
	}

	bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
	return InputClassToUse || bSuperResult;
}

void UAnimGraphNode_DynamicSubInstance::GetExposableProperties(TArray<UProperty*>& OutExposableProperties) const
{
	OutExposableProperties.Empty();

	UClass* TargetClass = *Node.InputClass;

	if (TargetClass)
	{
		const UEdGraphSchema_K2* Schema = CastChecked<UEdGraphSchema_K2>(GetSchema());

		for (TFieldIterator<UProperty> It(TargetClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			UProperty* CurProperty = *It;
			FEdGraphPinType PinType;

			if (CurProperty->HasAllPropertyFlags(CPF_Edit | CPF_BlueprintVisible) && CurProperty->HasAllFlags(RF_Public) && Schema->ConvertPropertyToPinType(CurProperty, PinType))
			{
				OutExposableProperties.Add(CurProperty);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE