async function createTable(pool) {

    console.log(`Creating table(-es) in DB 'Desbordante'`)

    // status -- COMPLETED/IN PROCESS/ERROR/NOT IN PROCESS
    return await pool.query(
        `CREATE TABLE IF NOT EXISTS tasks(\n
        taskID char(40) not null primary key,\n
        createdAt timestamp not null,\n
        algName char(10) not null,\n 
        errorPercent real not null,\n
        semicolon char(1) not null,\n
        progress real not null,\n
        status varchar(400) not null,\n
        datasetPath char(120) not null,\n
        FDs varchar(2000),
        hasHeader bool not null,
        maxLHS int not null,
        JsonArrayNameValue varchar(2000))
        `
    )
    .then(res => {
            if (res !== undefined) 
                console.log(`Table 'task' was successfully created.`)
            }
    )
    .catch(err => {
        console.log('Error with table creation')
        throw err
        }
    )
}

module.exports = createTable;