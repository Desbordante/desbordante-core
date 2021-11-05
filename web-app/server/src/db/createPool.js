const { Pool } = require('pg')
const pgtools = require('pgtools');

const config = {
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  port: process.env.DB_PORT,
  host: process.env.DB_HOST,
}

pgtools.createdb(config, process.env.DB_NAME, function (err, res) {
    if (err) {
        if(err.name === 'duplicate_database') {
            console.log(`Database '${process.env.DB_NAME}' already exists`)
        } else {
            console.log(err);
            process.exit();
        }
    } else {
        console.log(`Database '${process.env.DB_NAME}' was successfully created`);
        console.log(res);
    }
});

// pools uses environment variables
// for connection information
var pool = new Pool({
    user: process.env.DB_USER,
    host: process.env.DB_HOST,
    database: process.env.DB_NAME,
    password: process.env.DB_PASSWORD,
    port: process.env.DB_PORT,
})

// the pool will emit an error on behalf of any idle clients
// it contains if a backend error or network partition happens
pool.on('error', (err, client) => {
    console.error('Unexpected error on idle client', err)
    process.exit(-1)
})

console.log(`Connection to database '${process.env.DB_NAME}' established.`)

module.exports = pool