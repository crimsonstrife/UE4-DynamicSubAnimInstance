#include "DSAIRuntimePrivatePCH.h"
#include "AnimNode_DynamicSubInstance.h"
#include "AnimInstanceProxy.h"

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
	InstanceToRun = NewObject<UAnimInstance>(Mesh, InputClass);
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