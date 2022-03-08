#include "duckdb/function/scalar/list/aggregates.hpp"

#include "duckdb/planner/expression/bound_aggregate_expression.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/function/scalar/nested_functions.hpp"
#include "duckdb/planner/expression_binder.hpp"
#include "duckdb/catalog/catalog.hpp"
#include "duckdb/catalog/catalog_entry/aggregate_function_catalog_entry.hpp"
#include "duckdb/execution/expression_executor.hpp"

namespace duckdb {

// FIXME: use a local state for each thread to increase performance?
// FIXME: use update instead of simple_update to make use of 'group by functionality'
// this should also increase performance (especially for many small lists)

ListAggregatesBindData::ListAggregatesBindData(const LogicalType &stype_p, AggregateFunction aggr_functio_p)
    : stype(stype_p), aggr_function(aggr_functio_p) {
}

ListAggregatesBindData::~ListAggregatesBindData() {
}

unique_ptr<FunctionData> ListAggregatesBindData::Copy() {
	return make_unique<ListAggregatesBindData>(stype, aggr_function);
}

static void ListAggregateFunction(DataChunk &args, ExpressionState &state, Vector &result) {

	D_ASSERT(args.ColumnCount() == 2);
	auto count = args.size();
	Vector &lists = args.data[0];

	// get the aggregate function
	auto &func_expr = (BoundFunctionExpression &)state.expr;
	auto &info = (ListAggregatesBindData &)*func_expr.bind_info;
	auto &aggr = (BoundAggregateExpression &)info.aggr_function;

	// set the result vector
	result.SetVectorType(VectorType::FLAT_VECTOR);
	auto &result_validity = FlatVector::Validity(result);

	if (lists.GetType().id() == LogicalTypeId::SQLNULL) {
		result_validity.SetInvalid(0);
		return;
	}

	auto lists_size = ListVector::GetListSize(lists);
	auto &child_vector = ListVector::GetEntry(lists);

	VectorData child_data;
	child_vector.Orrify(lists_size, child_data);

	VectorData lists_data;
	lists.Orrify(count, lists_data);
	auto list_entries = (list_entry_t *)lists_data.data;

	// state_buffer holds the state for each list of this chunk
	idx_t size = aggr.function.state_size();
	auto state_buffer = unique_ptr<data_t[]>(new data_t[size * count]);

	// state_vector holds the pointers to the states
	Vector state_vector = Vector(LogicalType::POINTER, count);
	auto states = FlatVector::GetData<data_ptr_t>(state_vector);

	for (idx_t i = 0; i < count; i++) {
		auto lists_index = lists_data.sel->get_index(i);

		// initialize the aggregate state for this list
		states[i] = state_buffer.get() + size * i;
		aggr.function.initialize(states[i]);

		if (!lists_data.validity.RowIsValid(lists_index)) {
			result_validity.SetInvalid(i);
			continue;
		}

		const auto &list_entry = list_entries[lists_index];
		auto source_idx = child_data.sel->get_index(list_entry.offset);

		// update the aggregate state
		Vector list_slice = Vector(child_vector, source_idx);
		aggr.function.simple_update(&list_slice, aggr.bind_info.get(), 1, states[i], list_entry.length);
	}

	// finalize all the aggregate states
	aggr.function.finalize(state_vector, aggr.bind_info.get(), result, count, 0);
}

static unique_ptr<FunctionData> ListAggregateBind(ClientContext &context, ScalarFunction &bound_function,
                                                  vector<unique_ptr<Expression>> &arguments) {

	// the list column and the name of the aggregate function
	D_ASSERT(bound_function.arguments.size() == 2);
	D_ASSERT(arguments.size() == 2);

	if (arguments[0]->return_type.id() == LogicalTypeId::SQLNULL) {
		bound_function.arguments[0] = LogicalType::SQLNULL;
		bound_function.return_type = LogicalType::SQLNULL;
	} else {
		D_ASSERT(LogicalTypeId::LIST == arguments[0]->return_type.id());
		bound_function.return_type = ListType::GetChildType(arguments[0]->return_type);
	}

	if (!arguments[1]->IsFoldable()) {
		throw InvalidInputException("Aggregate function field must be a constant");
	}

	Value function_value = ExpressionExecutor::EvaluateScalar(*arguments[1]);
	auto function_name = StringValue::Get(function_value);

	// look up the aggregate function in the catalog
	QueryErrorContext error_context(nullptr, 0);
	auto func = (AggregateFunctionCatalogEntry *)Catalog::GetCatalog(context).GetEntry<AggregateFunctionCatalogEntry>(
	    context, DEFAULT_SCHEMA, function_name, false, error_context);
	D_ASSERT(func->type == CatalogType::AGGREGATE_FUNCTION_ENTRY);

	// find a matching aggregate function
	string error;
	vector<LogicalType> types;
	types.push_back(bound_function.return_type);
	auto best_function = Function::BindFunction(func->name, func->functions, types, error);
	if (best_function == DConstants::INVALID_INDEX) {
		throw BinderException("No matching aggregate function");
	}

	// found a matching function, bind it as an aggregate
	auto &best_bound_function = func->functions[best_function];
	unique_ptr<BoundAggregateExpression> bound_aggr_function = AggregateFunction::BindAggregateFunction(context, best_bound_function, {});
	unique_ptr<AggregateFunction> aggr_function = make_unique<AggregateFunction>(bound_aggr_function->function);

	bound_function.return_type = aggr_function->return_type;
	return make_unique<ListAggregatesBindData>(bound_function.return_type, *aggr_function);
}

ScalarFunction ListAggregateFun::GetFunction() {
	return ScalarFunction({LogicalType::LIST(LogicalType::ANY), LogicalType::VARCHAR}, LogicalType::ANY,
	                      ListAggregateFunction, false, ListAggregateBind, nullptr, nullptr, nullptr);
}

void ListAggregateFun::RegisterFunction(BuiltinFunctions &set) {
	set.AddFunction({"list_aggregate", "array_aggregate", "list_aggr", "array_aggr"}, GetFunction());
}

} // namespace duckdb
