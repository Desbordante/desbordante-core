var express = require('express');
var router = express.Router();

router.get('/', function(req, res, next) {

    const algs = ['Tane', 'Pyro']
    var result = [];

    for (var idx in algs) {
        result.push({name: algs[idx]});
    }

    res.send(JSON.stringify({algorithms : result}));
});

module.exports = router;
