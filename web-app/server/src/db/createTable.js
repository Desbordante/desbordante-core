async function createTable(pool) {

    console.log(`Creating table(-es) in DB 'Desbordante'`)

    // Table creation
    // status -- COMPLETED/PROCESS/ERROR/NOT IN PROCESS
    return await pool.query(
        `CREATE TABLE IF NOT EXISTS tasks(\n
        taskID char(40) not null primary key,\n
        createdAt timestamp not null,\n
        algName char(10) not null,\n 
        errorPercent real not null,\n
        semicolon char(1) not null,\n
        progress real not null,\n
        status varchar(400) not null,\n
        datasetPath char(100) not null)`
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

    // fileID Oid not null,\n
    // FDs varchar(2000)

    // Todo: add trigger (time-to-leave)   
}

module.exports = createTable;