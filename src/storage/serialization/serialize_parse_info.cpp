//===----------------------------------------------------------------------===//
// This file is automatically generated by scripts/generate_serialization.py
// Do not edit this file manually, your changes will be overwritten
//===----------------------------------------------------------------------===//

#include "duckdb/common/serializer/format_serializer.hpp"
#include "duckdb/common/serializer/format_deserializer.hpp"
#include "duckdb/parser/parsed_data/parse_info.hpp"
#include "duckdb/parser/parsed_data/alter_info.hpp"
#include "duckdb/parser/parsed_data/alter_table_info.hpp"

namespace duckdb {

void ParseInfo::FormatSerialize(FormatSerializer &serializer) const {
	serializer.WriteProperty("info_type", info_type);
}

unique_ptr<ParseInfo> ParseInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto info_type = deserializer.ReadProperty<ParseInfoType>("info_type");
	unique_ptr<ParseInfo> result;
	switch (info_type) {
	case ParseInfoType::ALTER_INFO:
		result = AlterInfo::FormatDeserialize(deserializer);
		break;
	default:
		throw SerializationException("Unsupported type for deserialization of ParseInfo!");
	}
	return result;
}

void AlterInfo::FormatSerialize(FormatSerializer &serializer) const {
	ParseInfo::FormatSerialize(serializer);
	serializer.WriteProperty("type", type);
	serializer.WriteProperty("catalog", catalog);
	serializer.WriteProperty("schema", schema);
	serializer.WriteProperty("name", name);
	serializer.WriteProperty("if_not_found", if_not_found);
	serializer.WriteProperty("allow_internal", allow_internal);
}

unique_ptr<ParseInfo> AlterInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto type = deserializer.ReadProperty<AlterType>("type");
	auto catalog = deserializer.ReadProperty<string>("catalog");
	auto schema = deserializer.ReadProperty<string>("schema");
	auto name = deserializer.ReadProperty<string>("name");
	auto if_not_found = deserializer.ReadProperty<OnEntryNotFound>("if_not_found");
	auto allow_internal = deserializer.ReadProperty<bool>("allow_internal");
	unique_ptr<AlterInfo> result;
	switch (type) {
	case AlterType::ALTER_TABLE:
		result = AlterTableInfo::FormatDeserialize(deserializer);
		break;
	case AlterType::ALTER_VIEW:
		result = AlterViewInfo::FormatDeserialize(deserializer);
		break;
	default:
		throw SerializationException("Unsupported type for deserialization of AlterInfo!");
	}
	result->catalog = std::move(catalog);
	result->schema = std::move(schema);
	result->name = std::move(name);
	result->if_not_found = if_not_found;
	result->allow_internal = allow_internal;
	return result;
}

void AlterTableInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterInfo::FormatSerialize(serializer);
	serializer.WriteProperty("alter_table_type", alter_table_type);
}

unique_ptr<AlterInfo> AlterTableInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto alter_table_type = deserializer.ReadProperty<AlterTableType>("alter_table_type");
	unique_ptr<AlterTableInfo> result;
	switch (alter_table_type) {
	case AlterTableType::ADD_COLUMN:
		result = AddColumnInfo::FormatDeserialize(deserializer);
		break;
	case AlterTableType::ALTER_COLUMN_TYPE:
		result = ChangeColumnTypeInfo::FormatDeserialize(deserializer);
		break;
	case AlterTableType::DROP_NOT_NULL:
		result = DropNotNullInfo::FormatDeserialize(deserializer);
		break;
	case AlterTableType::FOREIGN_KEY_CONSTRAINT:
		result = AlterForeignKeyInfo::FormatDeserialize(deserializer);
		break;
	case AlterTableType::REMOVE_COLUMN:
		result = RemoveColumnInfo::FormatDeserialize(deserializer);
		break;
	case AlterTableType::RENAME_COLUMN:
		result = RenameColumnInfo::FormatDeserialize(deserializer);
		break;
	case AlterTableType::RENAME_TABLE:
		result = RenameTableInfo::FormatDeserialize(deserializer);
		break;
	case AlterTableType::SET_DEFAULT:
		result = SetDefaultInfo::FormatDeserialize(deserializer);
		break;
	case AlterTableType::SET_NOT_NULL:
		result = SetNotNullInfo::FormatDeserialize(deserializer);
		break;
	default:
		throw SerializationException("Unsupported type for deserialization of AlterTableInfo!");
	}
	return result;
}

void AlterViewInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterInfo::FormatSerialize(serializer);
	serializer.WriteProperty("alter_view_type", alter_view_type);
}

unique_ptr<AlterInfo> AlterViewInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto alter_view_type = deserializer.ReadProperty<AlterViewType>("alter_view_type");
	unique_ptr<AlterViewInfo> result;
	switch (alter_view_type) {
	case AlterViewType::RENAME_VIEW:
		result = RenameViewInfo::FormatDeserialize(deserializer);
		break;
	default:
		throw SerializationException("Unsupported type for deserialization of AlterViewInfo!");
	}
	return result;
}

void AddColumnInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("new_column", new_column);
	serializer.WriteProperty("if_column_not_exists", if_column_not_exists);
}

unique_ptr<AlterTableInfo> AddColumnInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto new_column = deserializer.ReadProperty<ColumnDefinition>("new_column");
	auto result = duckdb::unique_ptr<AddColumnInfo>(new AddColumnInfo(std::move(new_column)));
	deserializer.ReadProperty("if_column_not_exists", result->if_column_not_exists);
	return std::move(result);
}

void AlterForeignKeyInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("fk_table", fk_table);
	serializer.WriteProperty("pk_columns", pk_columns);
	serializer.WriteProperty("fk_columns", fk_columns);
	serializer.WriteProperty("pk_keys", pk_keys);
	serializer.WriteProperty("fk_keys", fk_keys);
	serializer.WriteProperty("type", type);
}

unique_ptr<AlterTableInfo> AlterForeignKeyInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<AlterForeignKeyInfo>(new AlterForeignKeyInfo());
	deserializer.ReadProperty("fk_table", result->fk_table);
	deserializer.ReadProperty("pk_columns", result->pk_columns);
	deserializer.ReadProperty("fk_columns", result->fk_columns);
	deserializer.ReadProperty("pk_keys", result->pk_keys);
	deserializer.ReadProperty("fk_keys", result->fk_keys);
	deserializer.ReadProperty("type", result->type);
	return std::move(result);
}

void ChangeColumnTypeInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("column_name", column_name);
	serializer.WriteProperty("target_type", target_type);
	serializer.WriteProperty("expression", *expression);
}

unique_ptr<AlterTableInfo> ChangeColumnTypeInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<ChangeColumnTypeInfo>(new ChangeColumnTypeInfo());
	deserializer.ReadProperty("column_name", result->column_name);
	deserializer.ReadProperty("target_type", result->target_type);
	deserializer.ReadProperty("expression", result->expression);
	return std::move(result);
}

void DropNotNullInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("column_name", column_name);
}

unique_ptr<AlterTableInfo> DropNotNullInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<DropNotNullInfo>(new DropNotNullInfo());
	deserializer.ReadProperty("column_name", result->column_name);
	return std::move(result);
}

void RemoveColumnInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("removed_column", removed_column);
	serializer.WriteProperty("if_column_exists", if_column_exists);
	serializer.WriteProperty("cascade", cascade);
}

unique_ptr<AlterTableInfo> RemoveColumnInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<RemoveColumnInfo>(new RemoveColumnInfo());
	deserializer.ReadProperty("removed_column", result->removed_column);
	deserializer.ReadProperty("if_column_exists", result->if_column_exists);
	deserializer.ReadProperty("cascade", result->cascade);
	return std::move(result);
}

void RenameColumnInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("old_name", old_name);
	serializer.WriteProperty("new_name", new_name);
}

unique_ptr<AlterTableInfo> RenameColumnInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<RenameColumnInfo>(new RenameColumnInfo());
	deserializer.ReadProperty("old_name", result->old_name);
	deserializer.ReadProperty("new_name", result->new_name);
	return std::move(result);
}

void RenameTableInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("new_table_name", new_table_name);
}

unique_ptr<AlterTableInfo> RenameTableInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<RenameTableInfo>(new RenameTableInfo());
	deserializer.ReadProperty("new_table_name", result->new_table_name);
	return std::move(result);
}

void RenameViewInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterViewInfo::FormatSerialize(serializer);
	serializer.WriteProperty("new_view_name", new_view_name);
}

unique_ptr<AlterViewInfo> RenameViewInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<RenameViewInfo>(new RenameViewInfo());
	deserializer.ReadProperty("new_view_name", result->new_view_name);
	return std::move(result);
}

void SetDefaultInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("column_name", column_name);
	serializer.WriteProperty("expression", *expression);
}

unique_ptr<AlterTableInfo> SetDefaultInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<SetDefaultInfo>(new SetDefaultInfo());
	deserializer.ReadProperty("column_name", result->column_name);
	deserializer.ReadProperty("expression", result->expression);
	return std::move(result);
}

void SetNotNullInfo::FormatSerialize(FormatSerializer &serializer) const {
	AlterTableInfo::FormatSerialize(serializer);
	serializer.WriteProperty("column_name", column_name);
}

unique_ptr<AlterTableInfo> SetNotNullInfo::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<SetNotNullInfo>(new SetNotNullInfo());
	deserializer.ReadProperty("column_name", result->column_name);
	return std::move(result);
}

} // namespace duckdb
