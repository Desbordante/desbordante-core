var express = require('express');
var router = express.Router();

router.post('/upload', async (req, res) => {
    try {
        if(!req.files) {
            res.send({
                status: false,
                message: 'No file uploaded',
            });
        } else {
            console.log(req);
            // Use the name of the input field (i.e. "file") to retrieve the uploaded file
            let table = req.files.file;
            
            // Use the mv() method to place the file in upload directory (i.e. "uploads")
            table.mv('./uploads/' + table.name);
  
            // send response
            res.send({
                status: true,
                message: 'File is uploaded',
                data: {
                    name: table.name,
                    mimetype: table.mimetype,
                    size: table.size
                }
            });
        }
    } catch (err) {
        res.status(500).send(err);
    }
  }
);

module.exports = router;
