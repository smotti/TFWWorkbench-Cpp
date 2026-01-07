#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/Engine/UDataTable.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/Property/FNumericProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/FString.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>

#include <cstring>


constexpr size_t INVENTORY_ITEM_DETAILS_SIZE = 0x240;
const auto DATA_TABLE_NAME = STR("/Game/Blueprints/Data/ItemDetailsData.ItemDetailsData");


using namespace RC;
using namespace RC::Unreal;

class TFWWorkbench : public RC::CppUserModBase
{
private:
	// Static instance pointer for use in static lua callbacks
	static TFWWorkbench* s_instance;

	UDataTable* m_cached_data_table = nullptr;
	UScriptStruct* m_cached_row_struct = nullptr;

public:
	TFWWorkbench() : CppUserModBase()
	{
		ModName = STR("TFWWorkbench");
		ModVersion = STR("0.1");
		ModDescription = STR("Manage items, vendors, and more for TFW");
		ModAuthors = STR("terraru");

		Output::send<LogLevel::Verbose>(STR("TFWWorkbench says hello\n"));

		s_instance = this;
	}

	~TFWWorkbench() override
	{
		s_instance = nullptr;
	}

	auto on_update() -> void override
	{
	}

	auto on_unreal_init() -> void override
	{
		m_cached_data_table = static_cast<UDataTable*>(
			UObjectGlobals::StaticFindObject<UObject*>(
				nullptr,
				nullptr,
				DATA_TABLE_NAME
			)
		);

		if (m_cached_data_table)
		{
			m_cached_row_struct = m_cached_data_table->GetRowStruct();
			Output::send<LogLevel::Default>(
				STR("[TFWWorkbench] Cached DataTable and RowStruct successfully\n")
			);
		}
		else
		{
			Output::send<LogLevel::Warning>(
				STR("[TFWWorkbench] DataTable not found on init, will retry on function call\n")
			);
		}
	}

	auto on_lua_start(LuaMadeSimple::Lua& lua,
		LuaMadeSimple::Lua& main_lua,
		LuaMadeSimple::Lua& async_lua,
		LuaMadeSimple::Lua* hook_lua) -> void override
	{
			main_lua.register_function("AddInventoryItemRow", &TFWWorkbench::Lua_AddInventoryItemRow);
			main_lua.register_function("SetFTextPropertyValue", &TFWWorkbench::Lua_SetFTextPropertyValue);

			async_lua.register_function("AddInventoryItemRow", &TFWWorkbench::Lua_AddInventoryItemRow);
			async_lua.register_function("SetFTextPropertyValue", &TFWWorkbench::Lua_SetFTextPropertyValue);

			if (hook_lua)
			{
				hook_lua->register_function("AddInventoryItemRow", &TFWWorkbench::Lua_AddInventoryItemRow);
				hook_lua->register_function("SetFTextPropertyValue", &TFWWorkbench::Lua_SetFTextPropertyValue);
			}

			Output::send<LogLevel::Default>(STR("[TFWWorkbench] Registered Lua functions for mod\n"));
	}

private:
	auto GetDataTable() -> UDataTable*
	{
		if (!m_cached_data_table)
		{
			m_cached_data_table = static_cast<UDataTable*>(
				UObjectGlobals::StaticFindObject<UObject*>(
					nullptr,
					nullptr,
					DATA_TABLE_NAME
				)
			);
		}
		return m_cached_data_table;
	}

	static auto SetPropertyValueFromLua(const LuaMadeSimple::Lua& lua,
		LuaMadeSimple::LuaTableReference table,
		FProperty* property,
		void* propertyPtr,
		const std::wstring propertyName) -> void
	{
		if (auto* prop = CastField<FTextProperty>(property))
		{
			if (table.value.is_string())
			{
				auto propertyValue = to_wstring(table.value.get_string());
				*static_cast<FText*>(propertyPtr) = FText(propertyValue.c_str());

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set FText property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FStrProperty>(property))
		{
			if (table.value.is_string())
			{
				auto propertyValue = to_wstring(table.value.get_string());
				*static_cast<FString*>(propertyPtr) = FString(propertyValue.c_str());

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set FString property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FNameProperty>(property))
		{
			if (table.value.is_string())
			{
				auto propertyValue = to_wstring(table.value.get_string());
				*static_cast<FName*>(propertyPtr) = FName(propertyValue.c_str(), FNAME_Add);

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbecnh] Set FName property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FIntProperty>(property))
		{
			if (table.value.is_integer())
			{
				int64_t propertyValue = table.value.get_integer();
				*static_cast<int32*>(propertyPtr) = static_cast<int32>(propertyValue);

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set int32 property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FFloatProperty>(property))
		{
			if (table.value.is_number())
			{
				// That's a bit odd. Have to use get_number instead of get_float.
				// Otherwise the value would be 0.
				double propertyValue = table.value.get_number();
				*static_cast<float*>(propertyPtr) = static_cast<float>(propertyValue);

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set float property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FSoftObjectProperty>(property))
		{
			if (table.value.is_string())
			{
				auto propertyValue = to_wstring(table.value.get_string());
				FSoftObjectPath* softPath = static_cast<FSoftObjectPath*>(propertyPtr);
				softPath->AssetPathName = FName(propertyValue.c_str(), FNAME_Add);

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set TSoftObjectPtr property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FDoubleProperty>(property))
		{
			if (table.value.is_number())
			{
				double propertyValue = table.value.get_number();
				*static_cast<double*>(propertyPtr) = propertyValue;

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set double property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FEnumProperty>(property))
		{
			if (table.value.is_number())
			{
				double propertyValue = table.value.get_number();
				*static_cast<uint8*>(propertyPtr) = static_cast<uint8>(propertyValue);

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set FEnumProperty property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FArrayProperty>(property))
		{
			if (table.value.is_table())
			{
				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set FArrayProperty '{}'\n"),
					propertyName
				);

				FProperty* innerProp = prop->GetInner();
				int32 elementSize = innerProp->GetSize();

				// Count elements first
				int32 count = 0;
				lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference element) -> bool {
					count++;
					return false;
				});

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Creating array with {} elements\n"), count
				);

				auto* arr = static_cast<TArray<uint8>*>(propertyPtr);
				arr->Empty();

				if (count == 0) return;

				// SetName allocates and initializes
				arr->SetNum(count);

				uint8* data = arr->GetData();

				if (auto* innerStructProp = CastField<FStructProperty>(innerProp))
				{
					UScriptStruct* elementStruct = innerStructProp->GetStruct();
					for (int32 i = 0; i < count; i++)
					{
						elementStruct->InitializeStruct(data + (i * elementSize));
					}
				}

				// Fill from lua table
				int32 index = 0;
				lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference element) -> bool {
					void* elemPtr = data + (index * elementSize);

					// Array element is a StructProperty
					if (auto* innerStructProp = CastField<FStructProperty>(innerProp))
					{
						if (element.value.is_table())
						{
							UScriptStruct* elementStruct = innerStructProp->GetStruct();
							
							lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference field) -> bool {
								if (!field.key.is_string()) return false;

								auto fieldName = to_wstring(field.key.get_string());
								FProperty* fieldProp = elementStruct->GetPropertyByNameInChain(fieldName.c_str());

								if (fieldProp)
								{
									void* fieldPtr = fieldProp->ContainerPtrToValuePtr<void>(elemPtr);
									SetPropertyValueFromLua(lua, field, fieldProp, fieldPtr, fieldName);
								}

								return false;
							});
						}
					}

					index++;
					return false;
				});
			}
		}
		else if (auto* prop = CastField<FStructProperty>(property))
		{
			if (table.value.is_table())
			{
				auto innerStruct = prop->GetStruct();
				lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference innerTable) -> bool {
					if (!innerTable.key.is_string()) return false;

					auto innerPropertyName = to_wstring(innerTable.key.get_string());
					FProperty* innerProperty = innerStruct->GetPropertyByNameInChain(innerPropertyName.c_str());
					if (innerProperty)
					{
						void* innerPropertyPtr = innerProperty->ContainerPtrToValuePtr<void>(propertyPtr);
						SetPropertyValueFromLua(lua, innerTable, innerProperty, innerPropertyPtr, innerPropertyName);
					}
					return false;
				});
			}
		}
	}

	/// <summary>
	/// Lua function of three parameters:
	/// - string rowName
	/// - string propertyName
	/// - string propertyValue
	/// </summary>
	/// <param name="lua"></param>
	/// <returns></returns>
	static auto Lua_SetFTextPropertyValue(const LuaMadeSimple::Lua& lua) -> int
	{
		if (!s_instance)
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] No instance available\n"));
			lua.set_bool(false);
			return 1;
		}

		if (!lua.is_string(1) || !lua.is_string(2) || !lua.is_string(3))
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] Invalid parameters. Expected: (string, string, string)\n"));
			lua.set_bool(false);
			return 1;
		}

		auto rowName = to_wstring(lua.get_string());
		auto propertyName = to_wstring(lua.get_string());
		auto propertyValue = to_wstring(lua.get_string());

		UDataTable* dataTable = s_instance->GetDataTable();
		if (!dataTable)
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] DataTable not found: {}\n"), DATA_TABLE_NAME);
			lua.set_bool(false);
			return 1;
		}

		UScriptStruct* rowStruct = dataTable->GetRowStruct();
		if (!rowStruct)
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] DataTable RowStruct not found\n"));
			lua.set_bool(false);
			return 1;
		}

		FName rowFName(rowName.c_str(), FNAME_Find);
		uint8* row = dataTable->FindRowUnchecked(rowFName);
		if (!row)
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] Row '{}' not found\n"), rowName);
			lua.set_bool(false);
			return 1;
		}

		FProperty* property = rowStruct->GetPropertyByNameInChain(propertyName.c_str());
		if (!property)
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] Failed to find property '{}'\n"), propertyName);
			lua.set_bool(false);
			return 1;
		}
		FText* propertyPtr = property->ContainerPtrToValuePtr<FText>(row);
		propertyPtr->~FText();
		new (propertyPtr) FText(propertyValue.c_str());

		Output::send<LogLevel::Default>(STR("[TFWWorkbench] Setting property '{}' to value: {}\n"),
			propertyName, propertyValue);

		lua.set_bool(true);
		return 1;
	}

	static auto Lua_AddInventoryItemRow(const LuaMadeSimple::Lua& lua) -> int
	{
		if (!s_instance)
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] No instance available\n"));
			lua.set_bool(false);
			return 1;
		}

		try
		{

			std::string_view newRowName = lua.get_string();
			if (newRowName == "" || !lua.is_table())
			{
				Output::send<LogLevel::Error>(
					STR("[TFWWorkbench] Invalid parameters. Expected: (string, table)\n")
				);
				lua.set_bool(false);
				return 1;
			}
			LuaMadeSimple::Lua::Table rowData = lua.get_table();

			Output::send<LogLevel::Default>(STR("[TFWWorkbench] Adding row '{}'\n"),
				to_wstring(newRowName)
			);

			UDataTable* dataTable = s_instance->GetDataTable();
			if (!dataTable)
			{
				Output::send<LogLevel::Error>(STR("[TFWWorkbench] DataTable not found: {}\n"), DATA_TABLE_NAME);
				lua.set_bool(false);
				return 1;
			}

			UScriptStruct* rowStruct = dataTable->GetRowStruct();
			if (!rowStruct)
			{
				Output::send<LogLevel::Error>(STR("[TFWWorkbench] DataTable RowStruct not found\n"));
				lua.set_bool(false);
				return 1;
			}

			int32 structSize = rowStruct->GetPropertiesSize();
			uint8* newRow = static_cast<uint8*>(FMemory::Malloc(structSize, rowStruct->GetMinAlignment()));
			if (!newRow)
			{
				Output::send<LogLevel::Error>(STR("[TFWWorkbench] Failed to allocate memory for new row\n"));
				lua.set_bool(false);
				return 1;
			}

			rowStruct->InitializeStruct(newRow);
			FName sourceFName(STR("FirstAid"), FNAME_Find);
			uint8* sourceRow = dataTable->FindRowUnchecked(sourceFName);
			if (!sourceRow)
			{
				Output::send<LogLevel::Error>(STR("[TFWWorkbench] Source row FirstAid not found\n"));
				lua.set_bool(false);
				return 1;
			}
			rowStruct->CopyScriptStruct(newRow, sourceRow);

			lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
				if (!table.key.is_string()) return false;

				auto propertyName = to_wstring(table.key.get_string());
				Output::send<LogLevel::Verbose>(STR("[TFWWorkbench] Got property '{}'\n"), propertyName);
				FProperty* property = rowStruct->GetPropertyByNameInChain(propertyName.c_str());
				if (!property)
				{
					Output::send<LogLevel::Warning>(
						STR("[TFWWorkbench] Property '{}' not found, skipping\n"),
						propertyName
					);
					return false;
				}

				void* propertyPtr = property->ContainerPtrToValuePtr<void>(newRow);
				SetPropertyValueFromLua(lua, table, property, propertyPtr, propertyName);

				return false;
			});

			FName new_fname(to_wstring(newRowName).c_str(), FNAME_Add);
			dataTable->AddRow(new_fname, *reinterpret_cast<FTableRowBase*>(newRow));

			Output::send<LogLevel::Default>(STR("[TFWWorkbench] Successfully added row '{}'\n"), to_wstring(newRowName));

			// Cleanup
			rowStruct->DestroyStruct(newRow);
			FMemory::Free(newRow);

			lua.set_bool(true);
			return 1;
		}
		catch (const std::exception& e)
		{
			Output::send<LogLevel::Error>(
				STR("[TFWWorkbench] Exception: {}\n"),
				to_wstring(e.what())
			);
			lua.set_bool(false);
			return 1;
		}
	}
};

TFWWorkbench* TFWWorkbench::s_instance = nullptr;

#define TFWWORKBENCH_MOD_API __declspec(dllexport)
extern "C"
{
	TFWWORKBENCH_MOD_API RC::CppUserModBase* start_mod()
	{
		return new TFWWorkbench();
	}

	TFWWORKBENCH_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
	{
		delete mod;
	}
}