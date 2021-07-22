const express = require('express')
const cors = require('cors');
var logger = require('morgan');

var algorithmsRouter = require('./routes/algorithms.js');
var createTaskRouter = require('./routes/createTask');
// const { response } = require('express');

const app = express()
const jsonParser = express.json();


app.use(cors());
app.use(logger('dev'));
app.use(express.json());


// POST requests
app.post('/createTask', jsonParser, createTaskRouter);

// GET requests
app.use('/algorithms', algorithmsRouter);
app.use('/', (req, res) => {
  res.send('Hello World!')
});


// catch 404 and forward to error handler
app.use(function(req, res, next) {
  next(createError(404));
});

// error handler
app.use(function(err, req, res, next) {
  // set locals, only providing error in development
  res.locals.message = err.message;
  res.locals.error = req.app.get('env') === 'development' ? err : {};

  // render the error page
  res.status(err.status || 500);
  res.render('error');
});

module.exports = app;