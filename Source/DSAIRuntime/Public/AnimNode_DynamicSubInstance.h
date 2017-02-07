#pragma once

#include "Animation/AnimNode_SubInstance.h"
#include "AnimNode_DynamicSubInstance.generated.h"

USTRUCT()
struct DSAIRUNTIME_API FAnimNode_DynamicSubInstance : public FAnimNode_SubInstance
{
	GENERATED_BODY()

	public:
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (PinShownByDefault), Category = Links)
		TSubclassOf<UAnimInstance> InputClass;

		void Teardown(const UAnimInstance* InAnimInstance);
		void Reinitialize(const UAnimInstance* InAnimInstance);
		virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
		virtual void RootInitialize(const FAnimInstanceProxy* InProxy) override {}
};