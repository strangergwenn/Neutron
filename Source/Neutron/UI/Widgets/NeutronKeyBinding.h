// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "NeutronButton.h"

#include "GameFramework/PlayerInput.h"

DECLARE_DELEGATE_TwoParams(FNeutronOnKeyBindingChanged, FKey, FKey);

/** Key-binding storage structure that acts as a helper */
struct NEUTRON_API FNeutronKeyBinding
{
	FNeutronKeyBinding() : UserKey(FKey())
	{}

	FNeutronKeyBinding(const FText& InDisplayName) : UserKey(FKey()), DisplayName(InDisplayName)
	{}

	/** Add an action binding */
	FNeutronKeyBinding* Action(const FName& Mapping);

	/** Add an axis binding */
	FNeutronKeyBinding* Axis(const FName& Mapping, float Scale);

	/** Add a default key */
	FNeutronKeyBinding* Default(FKey InDefaultKey)
	{
		DefaultKey = InDefaultKey;
		return this;
	}

	/** Save this key binding to user configuration */
	void Save();

	/** Set the key */
	void SetKey(const FKey& Key)
	{
		UserKey = Key;
	}

	/** Reset the key to default */
	void ResetKey()
	{
		UserKey = DefaultKey;
	}

	/** Get the key */
	const FKey& GetKey() const
	{
		return UserKey;
	}

	/** Get the key name */
	FText GetKeyName() const
	{
		return UserKey.GetDisplayName();
	}

	/** Get the binding name */
	FText GetBindingName() const
	{
		return DisplayName;
	}

protected:

	FKey                           UserKey;
	FKey                           DefaultKey;
	FText                          DisplayName;
	TArray<FInputActionKeyMapping> ActionMappings;
	TArray<FInputAxisKeyMapping>   AxisMappings;
};

/** Key binding widget that behaves as a button */
class NEUTRON_API SNeutronKeyBinding : public SNeutronButton
{
public:

	SLATE_BEGIN_ARGS(SNeutronKeyBinding) : _Theme("DefaultButton"), _Icon(nullptr)
	{}

	SLATE_ARGUMENT(TSharedPtr<FNeutronKeyBinding>, Binding)

	SLATE_ARGUMENT(FName, Theme)

	SLATE_ARGUMENT(const FSlateBrush*, Icon)

	SLATE_EVENT(FNeutronOnKeyBindingChanged, OnKeyBindingChanged)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual FReply OnButtonClicked() override;

	/** Set the key that we will bind or rebind */
	void OnKeyPicked(FKey NewKey, bool bCanReset = true, bool bNotify = true);

	/** Key picking finished or canceled */
	void FinishWaiting();

protected:

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

	FText GetKeyName() const;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:

	/*----------------------------------------------------
	    Private data
	----------------------------------------------------*/

	// Data
	const FSlateBrush*             EditIcon;
	TSharedPtr<FNeutronKeyBinding> Binding;
	FNeutronOnKeyBindingChanged    OnKeyBindingChanged;
	FVector2D                      WaitingMousePos;
	bool                           WaitingForKey;
};
