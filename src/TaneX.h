// Strutovsky, 29.08.2019
#include <string>
#include "parser/CSVParser.h"

#pragma once

class Tane{
private:
  // some input fields - ignore for now
  CSVParser inputGenerator; //TODO: what do we do with this?
public:
  const std::string INPUT_FILE_CONFIG_KEY = "inputFile";
  // no evidence that there will be multiple Tane instances + tricky static stuff => just const
  long aprioriMillis = 0;
  void execute();
};
