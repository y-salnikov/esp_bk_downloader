const uint8 html[] =" <!DOCTYPE html>\n\
<html>\n\
 <head>\n\
  <meta charset=\"utf-8\">\n\
  <title>ESP BK Downloader</title>\n\
  <style>\n\
  </style>\n\
 </head>\n\
 <style>\n\
     #input_div\n\
     {\n\
         background-color: #EEDDFF;\n\
         border: 1px solid black;\n\
         padding: 30px;\n\
     }\n\
    #controls\n\
    {\n\
        visibility:hidden;\n\
        background-color: #D0DFD0;\n\
        border: 1px solid black;\n\
        border-radius: 15px;\n\
        padding: 30px;\n\
    }\n\
</style>\n\
 <body>\n\
     <div id=\"input_div\">\n\
         <input type=\"file\" id=\"files\" name=\"files[]\" />\n\
        <div id=\"controls\">\n\
            Имя файла:<input id=\"fn_txt\" type=\"text\"> <button type=\"button\" onclick=\"clear_name()\">Очистить</button> <br>\n\
            Адрес загрузки(8): <input id=\"adr_txt\" type=\"text\"> <br>\n\
            Длина данных: <input id=\"length_txt\" type=\"text\" readonly> <br>\n\
            Автозапуск: <input id=\"autostart\" type=\"checkbox\"> <br>\n\
            <center><button id=\"start_button\" type=\"button\" onclick=\"start_download()\" disabled> Начать загрузку </button></center>\n\
        </div>\n\
    </div>\n\
\n\
\n\
\n\
\n\
\n\
<script type=\"text/javascript\">\n\
     var stored_name=\"\";\n\
     var data=[];\n\
         function handleFileSelect(evt) \n\
         {\n\
                var files = evt.target.files; // FileList object\n\
\n\
                // files is a FileList of File objects. List some properties.\n\
                var output = [];\n\
                for (var i = 0, f; f = files[i]; i++)\n\
                {\n\
                      stored_name=f.name.toUpperCase();\n\
                      \n\
                }\n\
                var p=document.getElementById(\"controls\");\n\
                p.style.display=\"block\";\n\
                p.style.visibility=\"visible\";\n\
                load_file(files);\n\
             \n\
          }\n\
          document.getElementById('files').addEventListener('change', handleFileSelect, false);\n\
     \n\
     function load_file(files)\n\
     {\n\
         var f=files[0];\n\
         var myReader = new FileReader();\n\
         myReader.onload = function(event)\n\
         {\n\
             var data=new Uint8Array((myReader.result));\n\
             var arr=Array.prototype.slice.call(data);\n\
             window.data=arr;\n\
             var adr=window.data[0]+(window.data[1]*256);\n\
             var l=window.data[2]+(window.data[3]*256);\n\
             document.getElementById(\"start_button\").disabled=false;\n\
             document.getElementById(\"fn_txt\").value=stored_name;\n\
             document.getElementById(\"adr_txt\").value=adr.toString(8);\n\
             document.getElementById(\"length_txt\").value=l;\n\
             if(adr<512)\n\
             {\n\
                 document.getElementById(\"autostart\").checked=true;\n\
                 document.getElementById(\"autostart\").disabled=true;\n\
             }\n\
             if(adr>512)\n\
             {\n\
                  document.getElementById(\"autostart\").checked=false;\n\
                 document.getElementById(\"autostart\").disabled=true;\n\
              }\n\
             if(adr==512)\n\
             {\n\
                  document.getElementById(\"autostart\").checked=false;\n\
                 document.getElementById(\"autostart\").disabled=false;                 \n\
             }\n\
        };\n\
         myReader.readAsArrayBuffer(f);\n\
     }\n\
     \n\
     function clear_name()\n\
     {\n\
         document.getElementById(\"fn_txt\").value=\"\";\n\
     }\n\
     \n\
     function start_download()\n\
     {\n\
         var d=window.data;\n\
         send_file(d);\n\
     }\n\
     \n\
     function send_file(dat)\n\
     {\n\
         var chunksize=64;\n\
         var chunks=Math.ceil(dat.length/chunksize);\n\
         var remain=dat.length;\n\
         var adr=parseInt(document.getElementById(\"adr_txt\").value,8);\n\
         if (adr.isNaN) adr=512;\n\
         var l=parseInt(document.getElementById(\"length_txt\").value,10);\n\
         var d=dat.slice(4,dat.length);\n\
         if((document.getElementById(\"autostart\").checked==true) && (adr==512))\n\
         {\n\
             d=[0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2];\n\
             d=d.concat(dat.slice(4,dat.length));\n\
             adr=adr-16;\n\
             l=l+16;\n\
         }\n\
         var name=document.getElementById(\"fn_txt\").value;\n\
         send_header(adr,l,name);\n\
         \n\
         for(var i=0;i<chunks;i++)\n\
         {\n\
             var bytes_to_send=chunksize;\n\
             if(remain<chunksize) bytes_to_send=remain;\n\
             var dd=d.slice(i*chunksize,(i*chunksize)+bytes_to_send)\n\
             remain=remain-bytes_to_send;\n\
             send_chunk(dd);\n\
         }\n\
         send_end();\n\
         document.getElementById(\"controls\").style.visibility=\"hidden\";\n\
         \n\
     }\n\
     \n\
     function send_chunk(d)\n\
     {\n\
         var params=encodeURI(\"data=\"+'\"'+JSON.stringify(d)+'\"');\n\
         var dummy=\"\";\n\
         var xhr = new XMLHttpRequest();\n\
         xhr.open(\"GET\",\"data?\"+params,false);\n\
         xhr.setRequestHeader(\"Content-type\", \"application/x-www-form-urlencoded\");\n\
        xhr.send(dummy);\n\
     }\n\
     \n\
     function send_header(adr,length,name)\n\
     {\n\
         var params=encodeURI('adr='+adr+'&length='+length+'&name=\"'+name.toUpperCase()+'\"');\n\
         var dummy=\"\";\n\
         var xhr = new XMLHttpRequest();\n\
         xhr.open(\"GET\",\"header?\"+params,false);\n\
         xhr.setRequestHeader(\"Content-type\", \"application/x-www-form-urlencoded\");\n\
        xhr.send(dummy);\n\
     }\n\
     function send_end()\n\
     {\n\
         var dummy=\"\";\n\
         var xhr = new XMLHttpRequest();\n\
         xhr.open(\"GET\",\"end?end=1\",true);\n\
         xhr.setRequestHeader(\"Content-type\", \"application/x-www-form-urlencoded\");\n\
        xhr.send(dummy);\n\
     }\n\
     \n\
     </script>\n\
\n\
\n\
 </body>\n\
</html>\
\000\000\000";
