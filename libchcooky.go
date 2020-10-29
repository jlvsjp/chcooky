package main


import (
	"log"
	// "unsafe"
	"net/http"
	"io/ioutil"
	"encoding/json"
	"github.com/gorilla/websocket"
)

/*
#include <stdlib.h>
#include <string.h>
*/
import "C"

//export GetChromeCookiesFromWS
func GetChromeCookiesFromWS(wsurl *C.char) *C.char {
	c, _, err := websocket.DefaultDialer.Dial(C.GoString(wsurl), nil)
	if err != nil {
		log.Fatal("Error in Dial: ", err)
		return nil
	}
	defer c.Close()
	c.WriteMessage(websocket.TextMessage, []byte("{\"id\":1,\"method\":\"Network.getAllCookies\"}"))
	_, message, err := c.ReadMessage()
	if err != nil {
		log.Fatal("Error in ReadMessage: ", err)
		return nil
	}

	var msg map[string]interface{}
	err = json.Unmarshal(message, &msg)
	if err != nil {
		log.Fatal("Error in json.Unmarshal cookies: ", err)
		return nil
	}

	data := msg["result"].(map[string]interface {})
	data2, _ := json.Marshal(data["cookies"])

	// log.Printf("Cookies: %s", data2)
	// _cks := []byte(data2)
	// cookies := (*C.char)C.malloc((C.int)(len(data2)) + 1)
	// C.memset(cookies, (C.int)(0), (C.int)(len(data2)) + 1)
	// C.memcpy(cookies, (*C.char)(unsafe.Pointer(&_cks[0])), (C.int)(string(data2)))
	// return (*C.char)(unsafe.Pointer(&_cks[0]))
	return C.CString(string(data2))
}

//export GetChromeDBGUrl
func GetChromeDBGUrl(url *C.char) *C.char {
	resp, err := http.Get(C.GoString(url))
	if err != nil {
		// handle error
		log.Fatal("Error in http.Get: ", err)
		panic("")
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)

	// log.Printf("Resp body: %s", body)
	var dat []map[string]interface{}
	err = json.Unmarshal(body, &dat)

	if err != nil {
		log.Fatal("Error in json.Unmarshal: ", err)
		return nil
	}

	if (len(dat) == 0){
		return nil
	} else {
		wsurl := dat[0]["webSocketDebuggerUrl"].(string)
		return C.CString(wsurl)
	}
}


func main() {
	log.SetFlags(0)

	url := GetChromeDBGUrl(C.CString("http://localhost:43210/json"))
	log.Printf("webSocketDebuggerUrl: %s", C.GoString(url))

	// interrupt := make(chan os.Signal, 1)
	// signal.Notify(interrupt, os.Interrupt)
	cookies := GetChromeCookiesFromWS(url)
	log.Printf(C.GoString(cookies))
}
