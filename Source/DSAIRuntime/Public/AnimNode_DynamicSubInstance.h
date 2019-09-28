#pragma once

#include "Animation/AnimNode_SubInstance.h"
#include "AnimNode_DynamicSubInstance.generated.h"

USTRUCT()
struct DSAIRUNTIME_API FAnimNode_DynamicSubInstance : public FAnimNode_SubInstance
{
	GENERATED_BODY()

	public:
		FAnimNode_DynamicSubInstance();

		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (PinShownByDefault), Category = Links)
		TSubclassOf<UAnimInstance> InputClass;

		/** Optional tag used to identify this sub-instance */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (PinShownByDefault), Category = Links)
		FName ClassTag;

		virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;

//	protected:
//		virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override {}

	private:
		void Teardown(const UAnimInstance* InAnimInstance);
		void Reinitialize(const UAnimInstance* InAnimInstance);

};