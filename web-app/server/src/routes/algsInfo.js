var express = require('express');
var router = express.Router();

router.get('/', function(req, res, next) {

    const allowedFileFormats = ["text/csv", "application/vnd.ms-excel"]
    const allowedAlgorithms = ["Pyro", "TaneX", "FastFDs", "FD mine", "DFD"];
    const allowedSeparators = [",", "\\t", "\\n", "|", ";"];
    // TODO: Choose more suitable datasets
    const availableDatasets = ["BernoulliRelation.csv", "WDC_age.csv", "TestLong.csv", "TestWide.csv", "WDC_game.csv"];
    const maxFileSize = 50000000;
   
    res.send(JSON.stringify({ allowedFileFormats, allowedAlgorithms, allowedSeparators, availableDatasets, maxFileSize }));
});

module.exports = router;
