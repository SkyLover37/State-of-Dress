#pragma once

    static ImU32 C_SKYRIMGREY = IM_COL32(255, 255, 255, 100);
    static ImU32 C_SKYRIMWHITE = IM_COL32(255, 255, 255, 255);
    static ImU32 C_BLACK = IM_COL32(0, 0, 0, 255);
    static ImU32 C_SKYRIMDARKGREY_MENUBACKGROUND = IM_COL32(0, 0, 0, 125);
    static ImU32 C_QUARTERTRANSPARENT = IM_COL32(255, 255, 255, (int)(255.f * .25));
    static ImU32 C_HALFTRANSPARENT = IM_COL32(255, 255, 255, (int)(255.f * .5f));
    static ImU32 C_TRIQUARTERTRANSPARENT = IM_COL32(255, 255, 255, (int)(255.f * .75));
    static ImU32 C_VOID = IM_COL32(255, 255, 255, 0);

    namespace Styling {
        namespace Wheel {
            inline bool UseGeometricPrimitiveForBackgroundTexture = false;

            inline float CursorIndicatorDist = 10.f;  // distance from cusor indicator to the inner circle
            inline float CusorIndicatorArcWidth = 3.f;
            inline float CursorIndicatorArcAngle = 2 * IM_PI * 1 / 12.f;  // 1/12 of a circle
            inline float CursorIndicatorTriangleSideLength = 5.f;
            inline ImU32 CursorIndicatorColor = C_SKYRIMWHITE;
            inline bool CursorIndicatorInwardFacing = true;

            inline float WheelIndicatorOffsetX = 260.f;
            inline float WheelIndicatorOffsetY = 340.f;
            inline float WheelIndicatorSize = 10.f;
            inline float WheelIndicatorSpacing = 25.f;
            inline ImU32 WheelIndicatorActiveColor = C_SKYRIMWHITE;
            inline ImU32 WheelIndicatorInactiveColor = C_SKYRIMGREY;

            inline float InnerCircleRadius = 220.0f;
            inline float OuterCircleRadius = 360.0f;
            inline float InnerSpacing = 10.f;

            inline ImU32 HoveredColorBegin = C_QUARTERTRANSPARENT;
            inline ImU32 HoveredColorEnd = C_HALFTRANSPARENT;

            inline ImU32 UnhoveredColorBegin = C_SKYRIMDARKGREY_MENUBACKGROUND;
            inline ImU32 UnhoveredColorEnd = C_SKYRIMDARKGREY_MENUBACKGROUND;

            inline ImU32 ActiveArcColorBegin = C_SKYRIMWHITE;
            inline ImU32 ActiveArcColorEnd = C_SKYRIMWHITE;

            inline ImU32 InActiveArcColorBegin = C_SKYRIMGREY;
            inline ImU32 InActiveArcColorEnd = C_SKYRIMGREY;

            inline float ActiveArcWidth = 7.f;

            inline bool BlurOnOpen = true;
            inline float SlowTimeScale = .1f;

            // offset of wheel center, to which everything else is relative to
            inline float CenterOffsetX = 450.f;
            inline float CenterOffsetY = 0.f;

            inline ImU32 TextColor = C_SKYRIMWHITE;
            inline ImU32 TextShadowColor = C_BLACK;
        }

        namespace Entry {
            namespace Highlight {
                namespace Text {
                    inline float OffsetX = 0;
                    inline float OffsetY = -130;
                    inline float Size = 27;
                }
            }

        }

        namespace Item {
            namespace Highlight {
                namespace Texture {
                    inline float OffsetX = 0;
                    inline float OffsetY = -50;
                    inline float Scale = .2f;
                }

                namespace Text {
                    inline float OffsetX = 0;
                    inline float OffsetY = 20;
                    inline float Size = 35;
                }

                namespace Desc {
                    inline float OffsetX = 0;
                    inline float OffsetY = 50;
                    inline float Size = 30;
                    inline float LineLength = 500.f;
                    inline float LineSpacing = 5.f;
                }

                namespace StatIcon {
                    inline float OffsetX = 0;
                    inline float OffsetY = 0;
                    inline float Scale = .2f;
                }

                namespace StatText {
                    inline float OffsetX = 0;
                    inline float OffsetY = 0;
                    inline float Size = 35;
                }
            }
            namespace Slot {
                namespace Texture {
                    inline float OffsetX = 0;
                    inline float OffsetY = -25;
                    inline float Scale = .1f;
                }

                namespace Text {
                    inline float OffsetX = 0;
                    inline float OffsetY = 10;
                    inline float Size = 30;
                }

                namespace BackgroundTexture {
                    inline float Scale = .1f;
                }

            }

        }
    }
namespace Utils
{
	namespace Slot
	{
		RE::BGSEquipSlot* GetLeftHandSlot();
		RE::BGSEquipSlot* GetVoiceSlot();
		RE::BGSEquipSlot* GetRightHandSlot();
		void CleanSlot(RE::PlayerCharacter* a_pc, RE::BGSEquipSlot* a_slot);
	}

	namespace Time
	{
		float GGTM();
		void SGTM(float a_in);
	}

	namespace Magic
	{
		void GetMagicItemDescription(RE::ItemCard* a_itemCard, RE::MagicItem* a_magicItem, RE::BSString& a_str);

		void GetMagicItemDescription(RE::MagicItem* a_magicItem, std::string& a_buf);
	}

	namespace Inventory
	{
		std::pair<RE::EnchantmentItem*, float> GetEntryEnchantAndHealth(const std::unique_ptr<RE::InventoryEntryData>& a_invEntry);

		void GetEntryExtraDataLists(std::vector<RE::ExtraDataList*>& r_ret, const std::unique_ptr<RE::InventoryEntryData>& a_invEntry);
		
		enum class Hand
		{
			Left,
			Right,
			Both,
			None
		};
		Hand GetWeaponEquippedHand(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, uint32_t a_uniqueID, bool itemClean = false);

		RE::InventoryEntryData* GetSelectedItemIninventory(RE::InventoryMenu* a_invMenu);
		RE::TESForm* GetSelectedFormInMagicMenu(RE::MagicMenu* a_magMen);

		static inline RE::InventoryEntryData* sub_1401d5ba0(RE::InventoryEntryData* a_ptr, RE::TESBoundObject* a_obj, int count)
		{
			using func_t = RE::InventoryEntryData* (*)(RE::InventoryEntryData*, RE::TESBoundObject*, int);
			REL::Relocation<func_t> func{ RELOCATION_ID(10798, 10854) };
			return func(a_ptr, a_obj, count);
		}

		inline RE::InventoryEntryData* MakeInventoryEntryData(RE::TESBoundObject* a_obj)
		{
			RE::InventoryEntryData* ptr = (RE::InventoryEntryData*)RE::MemoryManager::GetSingleton()->Allocate(24, 0, true);
			ptr = sub_1401d5ba0(ptr, a_obj, 1);
			return ptr;
		}

	}
	
	namespace Workaround
	{
		inline void* NiMemAlloc_1400F6B40(int size)
		{
			using func_t = void* (*)(int);
			REL::Relocation<func_t> func{ RELOCATION_ID(10798, 10854) };
			return func(size);
		}
	}

	namespace Color
	{
		inline void MultAlpha(ImU32& a_u32, double a_mult)
		{
			a_u32 = (a_u32 & 0x00FFFFFF) | (static_cast<ImU32>(static_cast<double>(a_u32 >> 24) * a_mult) << 24);
		}
	};

	namespace Math
	{
		const RE::NiPoint3 HORIZONTAL_AXIS = { 0.0f, 0.0f, 1.0f }; 
		const RE::NiPoint3 VERTICAL_AXIS = { 1.0f, 0.0f, 0.0f };

		RE::NiMatrix3 MatrixFromAxisAngle(float theta, const RE::NiPoint3& axis = HORIZONTAL_AXIS);
	}

	void NotificationMessage(std::string a_message);

}
