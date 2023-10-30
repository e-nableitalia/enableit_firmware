const char serverIndex[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Update unsuccessful</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_50 {
        height: 50px;
      }
    </style>
  </head>
  <body>    
    <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.mi>.js'></script>
    <form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
        <input type='file' name='update'>
        <input type='submit' value='Update'>
    </form>
    <div id='prg'>progress: 0%</div>
    <script>
        $('form').submit(function(e) {
            e.preventDefault();
            var form = $('#upload_form')[0];
            var data = new FormData(form);
            $.ajax({
                url: '/update',
                type: 'POST',
                data: data,
                contentType: false,
                processData:false,
                xhr: function() {
                    var xhr = new window.XMLHttpRequest();
                    xhr.upload.addEventListener('progress', function(evt) {
                        if (evt.lengthComputable) {
                            var per = evt.loaded / evt.total;
                            $('#prg').html('progress: ' + Math.round(per*100) + '%');
                        }
                    }, false);
                    return xhr;
                },
                success:function(d, s) {
                    console.log('success!')
                },
                error: function (a, b, c) {
                }
            });
        });
    </script>
</body>
</html> )rawliteral";