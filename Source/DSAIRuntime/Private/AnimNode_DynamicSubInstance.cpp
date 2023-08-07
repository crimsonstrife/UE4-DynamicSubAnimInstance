#include "AnimNode_DynamicSubInstance.h"
#include "DSAIRuntimePrivatePCH.h"
#include "AnimInstanceProxy.h"

FAnimNode_DynamicSubInstance::FAnimNode_DynamicSubInstance()
	: InputClass(nullptr)
	, ClassTag(NAME_None)
{

}

void FAnimNode_DynamicSubInstance::Teardown(const UAnimInstance* InAnimInstance)
{
	USkeletalMeshComponent* Mesh = InAnimInstance->GetSkelMeshComponent();
	Mesh->SubInstances.Remove(InstanceToRun);
	InstanceToRun->UninitializeAnimation();
	InstanceToRun->MarkPendingKill();
	InstanceToRun = nullptr;
}

void FAnimNode_DynamicSubInstance::Reinitialize(const UAnimInstance* InAnimInstance)
{
	USkeletalMeshComponent* Mesh = InAnimInstance->GetSkelMeshComponent();

	// Need an instance to run, so create it now
	// We use the tag to name the object, but as we verify there are no duplicates in the compiler we
	// dont need to verify it is unique here.
	InstanceToRun = NewObject<UAnimInstance>(Mesh, InputClass, ClassTag);
	InstanceToRun->InitializeAnimation();

	Mesh->SubInstances.Add(InstanceToRun);
}

void FAnimNode_DynamicSubInstance::PreUpdate(const UAnimInstance* InAnimInstance)
{
	FAnimNode_SubInstance::PreUpdate(InAnimInstance);

	if (*InputClass)
	{
		if (InstanceToRun)
		{
			if (InstanceToRun->GetClass() != InputClass)
			{
				Teardown(InAnimInstance);
				Reinitialize(InAnimInstance);
			}
		}
		else
		{
			Reinitialize(InAnimInstance);
		}
	}
	else if (InstanceToRun)
	{
		Teardown(InAnimInstance);
	}
}