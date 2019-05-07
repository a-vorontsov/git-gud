package main

import (
	"bytes"
	"compress/gzip"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"regexp"
	"runtime"
	"strings"
	"time"

	"github.com/tidwall/gjson"
	"jaytaylor.com/html2text"
)

func runCmd() string {
	args := os.Args[1:]

	var stderr bytes.Buffer
	cmd := exec.Command("git", args...)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = &stderr
	if isWindows() {
		cmd.Run()
	} else {
		cmd.Start()
		cmd.Wait()
	}
	return string(stderr.Bytes())
}

func isWindows() bool {
	return runtime.GOOS == "windows" || detectWSL()
}

func detectWSL() bool {
	var detectedWSL bool
	var detectedWSLContents string
	if !detectedWSL {
		b := make([]byte, 1024)
		f, err := os.Open("/proc/version")
		if err == nil {
			f.Read(b)
			f.Close()
			detectedWSLContents = string(b)
		}
		detectedWSL = true
	}
	return strings.Contains(detectedWSLContents, "Microsoft")
}

func sendReq(path string) (res *http.Response, err error) {
	client := &http.Client{
		Transport: &http.Transport{
			DisableCompression: false,
		},
		Timeout: time.Second * 100,
	}
	req, err := http.NewRequest("GET", path, nil)

	if err != nil {
		log.Fatal(err)
	}

	req.Header.Set("Accept", "application/json")
	req.Header.Set("Accept-Encoding", "gzip")
	res, err = client.Do(req)
	if err != nil {
		log.Fatal(err)
	}
	return
}

func decompress(body io.ReadCloser) (output []byte, err error) {
	var reader io.ReadCloser
	reader, err = gzip.NewReader(body)
	if err != nil {
		log.Fatal(err)
	}
	defer reader.Close()

	output, err = ioutil.ReadAll(reader)
	return
}

func getQuestion(gitErr string) string {
	r, rerr := regexp.Compile(`(.*)[^exit status \d*](?m)`)
	if rerr != nil {
		log.Fatal(rerr)
	}

	rquestion := r.FindString(gitErr)
	question := url.QueryEscape(rquestion)
	reqPath := fmt.Sprintf("http://api.stackexchange.com/2.2/search/excerpts?key=U4DMV*8nvpm3EOpvf69Rxw((&pagesize=1&order=desc&sort=relevance&q=%s&site=stackoverflow&filter=!SWKo1XseJVmz_nSOm9", question)
	res, err := sendReq(reqPath)

	if err != nil {
		log.Fatal(err)
	}

	defer res.Body.Close()

	body, err := decompress(res.Body)
	if err != nil {
		log.Fatal(err)
	}

	item := gjson.Get(string(body), "items.0.question_id").String()
	return item
}

func getAnswer(question string) string {
	reqPath := fmt.Sprintf("http://api.stackexchange.com/2.2/questions/%s/answers?key=U4DMV*8nvpm3EOpvf69Rxw((&site=stackoverflow&page=1&pagesize=1&order=desc&sort=votes&filter=!Fcb(61J.xH8zQMnNMwf2k.*R8T", question)
	res, err := sendReq(reqPath)

	if err != nil {
		log.Fatal(err)
	}

	defer res.Body.Close()

	body, err := decompress(res.Body)
	if err != nil {
		log.Fatal(err)
	}

	answer := gjson.Get(string(body), "items.0.body").String()
	return answer
}

func formatHTML(html string) string {
	text, err := html2text.FromString(html, html2text.Options{PrettyTables: true})
	if err != nil {
		log.Fatal(err)
	}

	return text
}

func isIgnoreError(errMsg string) bool {
	return strings.Contains(errMsg, "empty commit message") || strings.Contains(errMsg, "not a git command")
}

func main() {
	gitError := runCmd()

	if gitError != "" && !isIgnoreError(gitError) {
		question := getQuestion(gitError)
		answer := getAnswer(question)
		text := formatHTML(answer)
		fmt.Println(text)
	} else {
		fmt.Println(gitError)
	}
}
