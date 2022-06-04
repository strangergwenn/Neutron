// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NeutronCaptureActor.generated.h"

/** Asset capture settings **/
struct NEUTRON_API FNeutronAssetPreviewSettings
{
	FNeutronAssetPreviewSettings();

	TSubclassOf<class AActor> Class;

	bool     RequireCustomPrimitives;
	FVector  Offset;
	FRotator Rotation;
	float    RelativeXOffset;
	float    Scale;
};

/** Camera control pawn for the factory view */
UCLASS(ClassGroup = (Neutron))
class ANeutronCaptureActor : public AActor
{
	GENERATED_BODY()

public:

	ANeutronCaptureActor();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	/** Render this asset and save it */
	void RenderAsset(class UNeutronAssetDescription* Asset, struct FSlateBrush& AssetRender);

protected:

#if WITH_EDITOR

	virtual void Tick(float DeltaTime) override;

	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	}

	/** Spawn the preview actor */
	void CreateActor(TSubclassOf<AActor> ActorClass);

	/** Get a catalog instance if not already existing */
	void CreateAssetManager();

	/** Spawn a new render target */
	void CreateRenderTarget();

	/** Set the camera to the ideal location */
	void ConfigureScene(const struct FNeutronAssetPreviewSettings& Settings);

	/** Save the render target to a texture */
	class UTexture2D* SaveTexture(FString TextureName);

	/** Get the texture desired size */
	FVector2D GetDesiredSize() const;

#endif    // WITH_EDITOR

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

#if WITH_EDITORONLY_DATA

public:

	// Upscale factor to apply to rendering
	UPROPERTY(Category = Neutron, EditDefaultsOnly)
	int32 RenderUpscaleFactor;

	// Upscale factor to apply to the image itself
	UPROPERTY(Category = Neutron, EditDefaultsOnly)
	int32 ResultUpscaleFactor;

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:

	// Camera arm
	UPROPERTY(Category = Neutron, VisibleDefaultsOnly, BlueprintReadOnly)
	class USceneComponent* CameraArmComponent;

	// Camera capture component
	UPROPERTY(Category = Neutron, VisibleDefaultsOnly, BlueprintReadOnly)
	class USceneCaptureComponent2D* CameraCapture;

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Asset to shoot
	UPROPERTY()
	UNeutronAssetDescription* AssetToShoot;

	// Preview actor
	UPROPERTY()
	class AActor* PreviewActor;

	// Asset manager
	UPROPERTY(Transient)
	class UNeutronAssetManager* AssetManager;

	// Render target used for rendering the assets
	UPROPERTY(Transient)
	class UTextureRenderTarget2D* RenderTarget;

	// Asset render
	FSlateBrush* TargetAssetRender;
	float        TimeBeforeScreenshot;

#endif    // WITH_EDITORONLY_DATA
};
