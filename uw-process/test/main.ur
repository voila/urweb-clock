fun main () = return <xml><body>
  <form> <submit value="smallbuf" action={smallbuf}/> </form>
  <form> <submit value="hugebuf" action={largebuf}/> </form>
</body></xml>


and smallbuf() = file("TEXT","cat", 2)

and largebuf() = file("TEXT","cat", 2000)

and file(stdin:string, cmd:string, bufsize) =
  result <- Process.exec "cat" (textBlob "TEXT") bufsize;
  return <xml><body>
      <div>
        stdin: {[stdin]}<br/>
        cmd: {[cmd]}<br/>
        bufsize: {[bufsize]}<br/>
        result as text: "{[Process.blobText (Process.blob result)]}"<br/>
        status:        {[Process.status result]}<br/>
        full:          {[Process.buf_full result]}<br/>
      </div>
  </body></xml>
