async function dropTableTasks(pool) {

    return await pool.query(`DROP TABLE IF EXISTS tasks`)
    .then((res) => {
        console.log(`Tables in DB 'desbordante' was successfully dropped`)
    })
    .catch((err) => {
        console.log('Problem with dropping table tasks')
        throw err
        }
    )
};

module.exports = dropTableTasks;