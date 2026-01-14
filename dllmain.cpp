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

using namespace RC;
using namespace RC::Unreal;

class TFWWorkbench : public RC::CppUserModBase
{
private:
	// Static instance pointer for use in static lua callbacks
	static TFWWorkbench* s_instance;

	std::map<std::string, UDataTable*> m_cached_data_table = {};
	std::map<std::string, UScriptStruct*> m_cached_row_struct = {};

	std::map<std::string, StringType> dataTables = {};
	std::map<std::string, StringType> dataTableSourceRows = {};

public:
	TFWWorkbench() : CppUserModBase()
	{
		ModName = STR("TFWWorkbench");
		ModVersion = STR("0.1");
		ModDescription = STR("Manage items, vendors, crafting recipes, and more for TFW");
		ModAuthors = STR("terraru");

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

	}

	auto on_lua_start(LuaMadeSimple::Lua& lua,
		LuaMadeSimple::Lua& main_lua,
		LuaMadeSimple::Lua& async_lua,
		LuaMadeSimple::Lua* hook_lua) -> void override
	{
		main_lua.register_function("AddDataTableRow", &TFWWorkbench::Lua_AddDataTableRow);
		main_lua.register_function("ConfigureDataTables", &TFWWorkbench::Lua_ConfigureDataTables);

		async_lua.register_function("AddDataTableRow", &TFWWorkbench::Lua_AddDataTableRow);
		async_lua.register_function("ConfigureDataTables", &TFWWorkbench::Lua_ConfigureDataTables);

		if (hook_lua)
		{
			hook_lua->register_function("AddDataTableRow", &TFWWorkbench::Lua_AddDataTableRow);
			hook_lua->register_function("ConfigureDataTables", &TFWWorkbench::Lua_AddDataTableRow);
		}

		Output::send<LogLevel::Default>(STR("[TFWWorkbench] Registered Lua functions for mod\n"));
	}

private:
	auto GetDataTable(std::string tableName) -> UDataTable*
	{
		if (!m_cached_data_table.contains(tableName))
		{
			try
			{
				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Caching DataTable: {}\n"),
					to_wstring(tableName)
				);
				m_cached_data_table[tableName] = static_cast<UDataTable*>(
					UObjectGlobals::StaticFindObject<UObject*>(
						nullptr,
						nullptr,
						dataTables.at(tableName)
					)
				);
			}
			catch (const std::out_of_range& ex)
			{
				Output::send<LogLevel::Error>(
					STR("[TFWWorkbench] Failed to cache DataTable: {}\n"),
					to_wstring(ex.what())
				);
				return nullptr;
			}
		}
		return m_cached_data_table[tableName];
	}

	auto GetDataTableRowStruct(std::string tableName) -> UScriptStruct*
	{
		if (!m_cached_row_struct.contains(tableName))
		{
			Output::send<LogLevel::Verbose>(
				STR("[TFWWorkbench] Caching RowStruct for DataTable: {}\n"), 
				to_wstring(tableName)
			);
			try
			{
				m_cached_row_struct[tableName] = this->GetDataTable(tableName)->GetRowStruct();
			}
			catch (const std::exception& ex)
			{
				Output::send<LogLevel::Error>(
					STR("[TFWWorkbench] Failed to cache RowStruct for DataTable {}: {}\n"),
					to_wstring(tableName),
					to_wstring(ex.what())
				);
				return nullptr;
			}
		}
		return m_cached_row_struct[tableName];
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
				FString quotedValue = FString::Printf(STR("NSLOCTEXT(\"\", \"\", \"%s\")"), propertyValue.c_str());
				prop->ImportText_Direct(quotedValue.GetCharArray().GetData(), propertyPtr, nullptr, PPF_None, nullptr);

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
		else if (auto* prop = CastField<FBoolProperty>(property))
		{
			if (table.value.is_bool())
			{
				bool propertyValue = table.value.get_bool();
				prop->SetPropertyValueInContainer(propertyPtr, propertyValue);

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set bool property '{}' to value: {}\n"),
					propertyName, propertyValue
				);
			}
		}
		else if (auto* prop = CastField<FSoftObjectProperty>(property))
		{
			if (table.value.is_string())
			{
				auto propertyValue = to_wstring(table.value.get_string());
				prop->ImportText_Direct(
					propertyValue.c_str(),
					propertyPtr,
					nullptr,
					PPF_None,
					nullptr
				);

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set {} property '{}' to value: {}\n"),
					prop->IsA<FSoftClassProperty>() ? STR("TSoftClassPr") : STR("TSoftObjectPtr"),
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
		else if (auto* prop = CastField<FObjectProperty>(property))
		{
			// The value passed from Lua is the full asset path as a string
			if (table.value.is_string())
			{
				auto propertyValue = to_wstring(table.value.get_string());
				UObject* obj = UObjectGlobals::StaticFindObject(
					nullptr,
					nullptr,
					propertyValue
				);

				if (obj)
				{
					Output::send<LogLevel::Verbose>(
						STR("[TFWWorkbench] Found object via path: {}\n"), propertyValue
					);
					*reinterpret_cast<UObject**>(propertyPtr) = obj;
				}
			}
		}
		else if (auto* prop = CastField<FMapProperty>(property))
		{
			if (table.value.is_table())
			{
				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Set FMapProperty '{}'\n"),
					propertyName
				);

				// Count map elements first
				int32 count = 0;
				lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference element) -> bool {
					count++;
					return false;
				});
				if (count == 0) return;
				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Creating map with {} elements\n"),
					count
				);

				FProperty* keyProp = prop->GetKeyProp();
				FProperty* valueProp = prop->GetValueProp();
				auto* map = static_cast<FScriptMap*>(propertyPtr);
				FScriptMapLayout scriptLayout = map->GetScriptLayout(
					keyProp->GetElementSize(), keyProp->GetMinAlignment(),
					valueProp->GetElementSize(), valueProp->GetMinAlignment());

				map->Empty(0, scriptLayout);

				// Only supports FName for key and FStruct for value
				lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference element) -> bool {
					int32 index = map->AddUninitialized(scriptLayout);
					uint8* entryData = static_cast<uint8*>(map->GetData(index, scriptLayout));
					void* keyPtr = entryData;
					void* valuePtr = entryData + scriptLayout.ValueOffset;

					if (auto* keyNameProp = CastField<FNameProperty>(keyProp))
					{
						if (element.key.is_string())
						{
							auto keyStr = to_wstring(element.key.get_string());
							*static_cast<FName*>(keyPtr) = FName(keyStr.c_str(), FNAME_Add);

							Output::send<LogLevel::Verbose>(
								STR("[TFWWorkbench] Map key: {}\n"), keyStr
							);
						}
					}

					if (auto* valueStructProp = CastField<FStructProperty>(valueProp))
					{
						UScriptStruct* valueStruct = valueStructProp->GetStruct();
						valueStruct->InitializeStruct(valuePtr);

						if (element.value.is_table())
						{
							lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference valueField) -> bool {
								if (!valueField.key.is_string()) return false;

								auto fieldName = to_wstring(valueField.key.get_string());
								FProperty* fieldProp = valueStruct->GetPropertyByNameInChain(fieldName.c_str());

								if (fieldProp)
								{
									void* fieldPtr = fieldProp->ContainerPtrToValuePtr<void>(valuePtr);
									SetPropertyValueFromLua(lua, valueField, fieldProp, fieldPtr, fieldName);
								}
								
								return false;
							});
						}
					}

					return false;
				});
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
				uint32 elementAligment = innerProp->GetMinAlignment();

				// Count elements first
				int32 count = 0;
				lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference element) -> bool {
					count++;
					return false;
				});

				Output::send<LogLevel::Verbose>(
					STR("[TFWWorkbench] Creating array with {} elements (elementSize={}, alignment={})\n"),
					count, elementSize, elementAligment
				);

				auto* arr = static_cast<FScriptArray*>(propertyPtr);
				arr->Empty(0, elementSize, elementAligment);

				if (count == 0) return;

				// Allocate and zero-initialize
				arr->AddZeroed(count, elementSize, elementAligment);

				uint8* data = static_cast<uint8*>(arr->GetData());

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
			/* This doesn't work. Either causes a crash on game startup or UE4SS crashing on dumping the table.
			* Probably fails in other instances too. When referencing the data.
			*
			else if (table.value.is_number())
			{
				auto innerStruct = prop->GetStruct();
				auto structName = innerStruct->GetName();
				// This is to handle the case of FTimespan not having a `Ticks` field.
				// Instead it's just 64 bit integer.
				// NOTE: There's probably a better way to check for the class /Script/CoreUObject.Timespan
				if (structName == STR("Timespan"))
				{
					int64_t propertyValue = table.value.get_integer();
					*static_cast<int64_t*>(propertyPtr) = propertyValue;

					Output::send<LogLevel::Verbose>(
						STR("[TFWWorkbench] Set Timespan property '{}' to value: {}\n"),
						propertyName, propertyValue
					);
				}
			}
			*/
		}
	}

	static auto Lua_AddDataTableRow(const LuaMadeSimple::Lua& lua) -> int
	{
		if (!s_instance)
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] No instance available\n"));
			lua.set_bool(false);
			return 1;
		}

		try
		{
			std::string_view tableName = lua.get_string();
			std::string_view newRowName = lua.get_string();
			if (tableName == "" || newRowName == "" || !lua.is_table())
			{
				Output::send<LogLevel::Error>(
					STR("[TFWWorkbench] Invalid parameters. Expected: (string, string, table)\n")
				);
				lua.set_bool(false);
				return 1;
			}

			UDataTable* dataTable = s_instance->GetDataTable(static_cast<std::string>(tableName));
			if (!dataTable)
			{
				Output::send<LogLevel::Error>(STR("[TFWWorkbench] DataTable not found: {}\n"), to_wstring(tableName));
				lua.set_bool(false);
				return 1;
			}

			UScriptStruct* rowStruct;
			try
			{
				rowStruct = s_instance->GetDataTableRowStruct(static_cast<std::string>(tableName));
			}
			catch (...)
			{
				rowStruct = dataTable->GetRowStruct();
			}

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

			// NOTE: Still haven't figured out why dumping (reading) of tables fail
			// if a "sourceRow" isn't used. It's successful in adding the new row w/o
			// a source row. But fails when reading the new row from the table.
			rowStruct->InitializeStruct(newRow);
			auto sourceRowName = s_instance->dataTableSourceRows.at(static_cast<std::string>(tableName));
			FName sourceFName(sourceRowName, FNAME_Find);
			uint8* sourceRow = dataTable->FindRowUnchecked(sourceFName);
			if (!sourceRow)
			{
				Output::send<LogLevel::Error>(STR("[TFWWorkbench] Source row {} not found\n"), sourceRowName);
				lua.set_bool(false);
				return 1;
			}
			
			rowStruct->CopyScriptStruct(newRow, sourceRow);

			Output::send<LogLevel::Default>(STR("[TFWWorkbench] Adding row '{}'\n"),
				to_wstring(newRowName)
			);

			FName new_fname(to_wstring(newRowName).c_str(), FNAME_Add);
			dataTable->AddRow(new_fname, *reinterpret_cast<FTableRowBase*>(newRow));
			uint8* actualRow = dataTable->FindRowUnchecked(new_fname);
			if (!actualRow)
			{
				Output::send<LogLevel::Error>(STR("[TFWWorkbench] Failed to find newly added row\n"));
				lua.set_bool(false);
				return 1;
			}

			lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
				if (!table.key.is_string()) return false;

				int stackBefore = lua_gettop(lua.get_lua_state());

				auto propertyName = to_wstring(table.key.get_string());
				Output::send<LogLevel::Verbose>(STR("[TFWWorkbench] Processing field '{}', stack depth: {}\n"),
					propertyName, stackBefore);
				//Output::send<LogLevel::Verbose>(STR("[TFWWorkbench] Got property '{}'\n"), propertyName);
				FProperty* property = rowStruct->GetPropertyByNameInChain(propertyName.c_str());
				if (!property)
				{
					Output::send<LogLevel::Warning>(
						STR("[TFWWorkbench] Property '{}' not found, skipping\n"),
						propertyName
					);
					return false;
				}

				void* propertyPtr = property->ContainerPtrToValuePtr<void>(actualRow);
				SetPropertyValueFromLua(lua, table, property, propertyPtr, propertyName);

				int stackAfter = lua_gettop(lua.get_lua_state());
				if (stackBefore != stackAfter)
				{
					Output::send<LogLevel::Error>(
						STR("[TFWWorkbench] STACK IMBALANCE after '{}: before={}, after={}\n"),
						propertyName, stackBefore, stackAfter
					);
				}

				return false;
			});

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

	static auto Lua_ConfigureDataTables(const LuaMadeSimple::Lua& lua) -> int
	{
		if (!s_instance)
		{
			Output::send<LogLevel::Error>(STR("[TFWWorkbench] No instance available\n"));
			lua.set_bool(false);
			return 1;
		}

		const int argCount = lua_gettop(lua.get_lua_state());
		if (argCount < 3)
		{
			Output::send<LogLevel::Error>(
				STR("[TFWWorkbench] Expected 3 arguments, got {}\n"),
				argCount
			);
			lua.set_bool(false);
			return 1;
		}

		lua_State* L = lua.get_lua_state();
		if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3))
		{
			Output::send<LogLevel::Error>(
				STR("[TFWWorkbench] Invalid parameter types. Expected: (string, string, string)\n")
			);
			lua.set_bool(false);
			return 1;
		}

		std::string tableName = lua_tostring(L, 1);
		StringType tablePath = to_wstring(lua_tostring(L, 2));
		StringType sourceRowName = to_wstring(lua_tostring(L, 3));

		if (tableName == "" || tablePath.empty() || sourceRowName.empty())
		{
			Output::send<LogLevel::Error>(
				STR("[TFWWorkbench] Parameters cannot be null or empty\n")
			);
			lua.set_bool(false);
			return 1;
		}

		Output::send<LogLevel::Verbose>(
			STR("[TFWWorkbench] Configuring DataTable: {} | {} | {}\n"),
			to_wstring(tableName), tablePath, sourceRowName
		);

		try
		{
			s_instance->dataTables.insert({ tableName, tablePath });
		}
		catch (const std::exception& ex)
		{
			Output::send<LogLevel::Error>(
				STR("[TFWWorkbench] Failed to configure data table: {} - {}\n"),
				to_wstring(tableName), tablePath
			);
			Output::send<LogLevel::Error>(
				STR("[TFWWorkbench] {}\n"), to_wstring(ex.what())
			);
			lua.set_bool(false);
			return 1;
		}

		try
		{
			s_instance->dataTableSourceRows.insert({ tableName, sourceRowName });
		}
		catch (const std::exception& ex)
		{
			Output::send<LogLevel::Error>(
				STR("[TFWWorkbench] Failed to configure source row for data table: {} - {}\n"),
				to_wstring(tableName), sourceRowName
			);
			Output::send<LogLevel::Error>(
				STR("[TFWWorkbench] {}\n"), to_wstring(ex.what())
			);
			lua.set_bool(false);
			return 1;
		}

		lua.set_bool(true);
		return 1;
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