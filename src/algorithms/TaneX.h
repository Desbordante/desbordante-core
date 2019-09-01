// Strutovsky, 29.08.2019
#include <string>
#include "../parser/CSVParser.h"

#pragma once

class Tane{
private:
  // some input fields - ignore for now
  CSVParser inputGenerator; //TODO: what do we do with this?
public:
  const std::string INPUT_FILE_CONFIG_KEY = "inputFile";

  const double fdErrorMeasure = 0.1; //TODO: these consts should go in class (or struct) Configuration
  const double maxFdError = 0.05;
  const double uccErrorMeasure = 0.1;
  const double maxUccError = 0.05;
  const int maxArity = 10;

    // no evidence that there will be multiple Tane instances + tricky static stuff => just const
  long aprioriMillis = 0;
  void execute();
};
