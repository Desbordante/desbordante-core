const { Pool } = require('pg')

// pools uses environment variables
// for connection information
var pool = new Pool({
    user: process.env.DB_USER,
    host: process.env.DB_HOST,
    database: process.env.DB_NAME,
    password: process.env.DB_PASSWORD,
    port: process.env.DB_PORT,
})

console.log(`Connect to database 'Desbordante'..`)

// the pool will emit an error on behalf of any idle clients
// it contains if a backend error or network partition happens
pool.on('error', (err, client) => {
    console.error('Unexpected error on idle client', err)
    process.exit(-1)
})

console.log(`Connection to database 'Desbordante' established.`)

module.exports = pool