// Strutovsky, 29.08.2019

#pragma once

#include <string>
#include "../parser/CSVParser.h"
#include "../util/PositionListIndex.h"
#include "../model/RelationData.h"


class Tane{
private:
  // some input fields - ignore for now
  CSVParser inputGenerator;
public:
  const std::string INPUT_FILE_CONFIG_KEY = "inputFile";

  //const double fdErrorMeasure = 0.1; //TODO: these consts should go in class (or struct) Configuration
  const double maxFdError = 0.05;
  //const double uccErrorMeasure = 0;
  const double maxUccError = 0.05;
  const int maxArity = 10;
  //TODO: DO NOT FORGET ABOUT MAXARITY!!
  // no evidence that there will be multiple Tane instances + tricky static stuff => just const

  long aprioriMillis = 0;
  explicit Tane(fs::path& path) : inputGenerator(path) {}
  explicit Tane(fs::path&& path) : inputGenerator(path) {}
  void execute();

  static double calculateZeroAryFdError(shared_ptr<ColumnData> rhs, shared_ptr<ColumnLayoutRelationData> relationData);
  static double calculateFdError(shared_ptr<PositionListIndex> lhsPli, shared_ptr<PositionListIndex> jointPli, shared_ptr<ColumnLayoutRelationData> relationData);
  static double calculateUccError(shared_ptr<PositionListIndex> pli, shared_ptr<ColumnLayoutRelationData> relationData);
  //static double round(double error) { return ((int)(error * 32768) + 1)/ 32768.0; }
  //can't say if this method is necessary -- TODO: use this function?
  static void registerFD(Vertical& lhs, shared_ptr<Column> rhs, double error);
  static void registerFD(shared_ptr<Vertical> lhs, shared_ptr<Column> rhs, double error);
  static void registerUCC(Vertical& key, double error);

};
