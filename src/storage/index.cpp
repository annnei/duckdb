#include "duckdb/storage/index.hpp"

#include "duckdb/common/radix.hpp"
#include "duckdb/planner/expression/bound_columnref_expression.hpp"
#include "duckdb/planner/expression/bound_reference_expression.hpp"
#include "duckdb/planner/expression_iterator.hpp"
#include "duckdb/storage/table/append_state.hpp"

namespace duckdb {

Index::Index(const string &name, const IndexType &index_type, IndexConstraintType index_constraint_type,
             const vector<column_t> &column_ids, TableIOManager &table_io_manager,
             const vector<unique_ptr<Expression>> &unbound_expressions, AttachedDatabase &db)

    : name(name), index_type(index_type), index_constraint_type(index_constraint_type), column_ids(column_ids),
      table_io_manager(table_io_manager), db(db) {

	if (!Radix::IsLittleEndian()) {
		throw NotImplementedException("indexes are not supported on big endian architectures");
	}

	for (auto &expr : unbound_expressions) {
		types.push_back(expr->return_type.InternalType());
		logical_types.push_back(expr->return_type);
		auto unbound_expression = expr->Copy();
		bound_expressions.push_back(BindExpression(unbound_expression->Copy()));
		this->unbound_expressions.emplace_back(std::move(unbound_expression));
	}
	for (auto &bound_expr : bound_expressions) {
		executor.AddExpression(*bound_expr);
	}

	// create the column id set
	column_id_set.insert(column_ids.begin(), column_ids.end());
}

void Index::InitializeLock(IndexLock &state) {
	state.index_lock = unique_lock<mutex>(lock);
}

PreservedError Index::Append(DataChunk &entries, Vector &row_identifiers) {
	IndexLock state;
	InitializeLock(state);
	return Append(state, entries, row_identifiers);
}

void Index::CommitDrop() {
	IndexLock index_lock;
	InitializeLock(index_lock);
	CommitDrop(index_lock);
}

void Index::Delete(DataChunk &entries, Vector &row_identifiers) {
	IndexLock state;
	InitializeLock(state);
	Delete(state, entries, row_identifiers);
}

bool Index::MergeIndexes(Index &other_index) {
	IndexLock state;
	InitializeLock(state);
	return MergeIndexes(state, other_index);
}

string Index::VerifyAndToString(const bool only_verify) {
	IndexLock state;
	InitializeLock(state);
	return VerifyAndToString(state, only_verify);
}

void Index::Vacuum() {
	IndexLock state;
	InitializeLock(state);
	Vacuum(state);
}

idx_t Index::GetInMemorySize() {
	IndexLock state;
	InitializeLock(state);
	return GetInMemorySize(state);
}

void Index::ExecuteExpressions(DataChunk &input, DataChunk &result) {
	executor.Execute(input, result);
}

unique_ptr<Expression> Index::BindExpression(unique_ptr<Expression> expr) {
	if (expr->type == ExpressionType::BOUND_COLUMN_REF) {
		auto &bound_colref = expr->Cast<BoundColumnRefExpression>();
		return make_uniq<BoundReferenceExpression>(expr->return_type, column_ids[bound_colref.binding.column_index]);
	}
	ExpressionIterator::EnumerateChildren(
	    *expr, [this](unique_ptr<Expression> &expr) { expr = BindExpression(std::move(expr)); });
	return expr;
}

bool Index::IndexIsUpdated(const vector<PhysicalIndex> &column_ids_p) const {
	for (auto &column : column_ids_p) {
		if (column_id_set.find(column.index) != column_id_set.end()) {
			return true;
		}
	}
	return false;
}

IndexStorageInfo Index::GetStorageInfo(const bool get_buffers) {
	throw NotImplementedException("The implementation of this index serialization does not exist.");
}

string Index::AppendRowError(DataChunk &input, idx_t index) {
	string error;
	for (idx_t c = 0; c < input.ColumnCount(); c++) {
		if (c > 0) {
			error += ", ";
		}
		error += input.GetValue(c, index).ToString();
	}
	return error;
}

//===--------------------------------------------------------------------===//
// Stable storage functions
//===--------------------------------------------------------------------===//

void IndexStorage::WriteDummyBlockPointers(Serializer &serializer) {

	vector<BlockPointer> index_pointers;
	index_pointers.emplace_back(INVALID_BLOCK, 42);
	serializer.WriteProperty(103, "index_pointers", index_pointers);
}

BlockPointer IndexStorage::GetBlockPointer(const IndexStorageInfo &info) {

	D_ASSERT(info.properties.find("block_id") != info.properties.end());
	D_ASSERT(info.properties.find("offset") != info.properties.end());

	auto block_id = info.properties.find("block_id")->second;
	auto offset = info.properties.find("offset")->second;

	BlockPointer block_pointer(block_id.GetValue<block_id_t>(), offset.GetValue<uint32_t>());
	return block_pointer;
}

void IndexStorage::SetBlockPointerInfos(vector<BlockPointer> &pointers, vector<IndexStorageInfo> &infos) {

	for (idx_t i = 0; i < pointers.size(); i++) {
		IndexStorageInfo index_storage_info;
		auto block_id = make_pair<string, Value>("block_id", Value::BIGINT(pointers[i].block_id));
		auto offset = make_pair<string, Value>("offset", Value::UINTEGER(pointers[i].offset));
		index_storage_info.properties.insert(block_id);
		index_storage_info.properties.insert(offset);
		infos.push_back(index_storage_info);
	}
}

void IndexStorage::SetBlockPointerInfo(BlockPointer &pointer, IndexStorageInfo &info) {

	auto block_id = make_pair<string, Value>("block_id", Value::BIGINT(pointer.block_id));
	auto offset = make_pair<string, Value>("offset", Value::UINTEGER(pointer.offset));
	info.properties.insert(block_id);
	info.properties.insert(offset);
}

} // namespace duckdb
