#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include "catalog/catalog_defs.h"
#include "catalog/schema.h"
#include "parser/expression/abstract_expression.h"
#include "parser/expression/column_value_expression.h"
#include "planner/plannodes/abstract_plan_node.h"
#include "planner/plannodes/abstract_scan_plan_node.h"

namespace terrier::planner {

/**
 * Plan node for sequanial table scan
 */
class SeqScanPlanNode : public AbstractScanPlanNode {
 public:
  /**
   * Builder for a sequential scan plan node
   */
  class Builder : public AbstractScanPlanNode::Builder<Builder> {
   public:
    Builder() = default;

    /**
     * Don't allow builder to be copied or moved
     */
    DISALLOW_COPY_AND_MOVE(Builder);

    /**
     * @param oid oid for table to scan
     * @return builder object
     */
    Builder &SetTableOid(catalog::table_oid_t oid) {
      table_oid_ = oid;
      return *this;
    }

    /**
     * @param column_oids OIDs of columns to scan
     * @return builder object
     */
    Builder &SetColumnOids(std::vector<catalog::col_oid_t> &&column_oids) {
      column_oids_ = std::move(column_oids);
      return *this;
    }

    /**
     * Build the sequential scan plan node
     * @return plan node
     */
    std::unique_ptr<SeqScanPlanNode> Build() {
      return std::unique_ptr<SeqScanPlanNode>(
          new SeqScanPlanNode(std::move(children_), std::move(output_schema_), scan_predicate_, std::move(column_oids_),
                              is_for_update_, database_oid_, namespace_oid_, table_oid_));
    }

   protected:
    /**
     * OIDs of columns to scan
     */
    std::vector<catalog::col_oid_t> column_oids_;

    /**
     * OID for table being scanned
     */
    catalog::table_oid_t table_oid_;
  };

 private:
  /**
   * @param children child plan nodes
   * @param output_schema Schema representing the structure of the output of this plan node
   * @param predicate scan predicate
   * @param column_oids OIDs of columns to scan
   * @param is_for_update flag for if scan is for an update
   * @param database_oid database oid for scan
   * @param table_oid OID for table to scan
   */
  SeqScanPlanNode(std::vector<std::unique_ptr<AbstractPlanNode>> &&children,
                  std::unique_ptr<OutputSchema> output_schema,
                  common::ManagedPointer<parser::AbstractExpression> predicate,
                  std::vector<catalog::col_oid_t> &&column_oids, bool is_for_update, catalog::db_oid_t database_oid,
                  catalog::namespace_oid_t namespace_oid, catalog::table_oid_t table_oid)
      : AbstractScanPlanNode(std::move(children), std::move(output_schema), predicate, is_for_update, database_oid,
                             namespace_oid),
        column_oids_(std::move(column_oids)),
        table_oid_(table_oid) {}

 public:
  /**
   * Default constructor used for deserialization
   */
  SeqScanPlanNode() = default;

  DISALLOW_COPY_AND_MOVE(SeqScanPlanNode)

  /**
   * @return the type of this plan node
   */
  PlanNodeType GetPlanNodeType() const override { return PlanNodeType::SEQSCAN; }

  /**
   * @return OIDs of columns to scan
   */
  const std::vector<catalog::col_oid_t> &GetColumnIds() const { return column_oids_; }

  /**
   * @return the OID for the table being scanned
   */
  catalog::table_oid_t GetTableOid() const { return table_oid_; }

  /**
   * @return the hashed value of this plan node
   */
  common::hash_t Hash() const override;

  bool operator==(const AbstractPlanNode &rhs) const override;

  nlohmann::json ToJson() const override;
  std::vector<std::unique_ptr<parser::AbstractExpression>> FromJson(const nlohmann::json &j) override;

  /**
   * Collect all column oids in this expression
   * @return the vector of unique columns oids
   */
  std::vector<catalog::col_oid_t> CollectInputOids() const {
    std::vector<catalog::col_oid_t> result;
    // Scan predicate
    if (GetScanPredicate() != nullptr) CollectOids(&result, GetScanPredicate().Get());
    // Output expressions
    for (const auto &col : GetOutputSchema()->GetColumns()) {
      CollectOids(&result, col.GetExpr().Get());
    }
    // Remove duplicates
    std::unordered_set<catalog::col_oid_t> s(result.begin(), result.end());
    result.assign(s.begin(), s.end());
    return result;
  }

 private:
  void CollectOids(std::vector<catalog::col_oid_t> *result, const parser::AbstractExpression *expr) const {
    if (expr->GetExpressionType() == parser::ExpressionType::COLUMN_VALUE) {
      auto column_val = static_cast<const parser::ColumnValueExpression *>(expr);
      result->emplace_back(column_val->GetColumnOid());
    } else {
      for (const auto &child : expr->GetChildren()) {
        CollectOids(result, child.Get());
      }
    }
  }

  /**
   * OIDs of columns to scan
   */
  std::vector<catalog::col_oid_t> column_oids_;

  /**
   * OID for table being scanned
   */
  catalog::table_oid_t table_oid_;
};

DEFINE_JSON_DECLARATIONS(SeqScanPlanNode);

}  // namespace terrier::planner
