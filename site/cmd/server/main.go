package main

import (
	"fmt"
	"net/http"

	"site/views"

	"github.com/a-h/templ"
)

func aaaaa() string {
	return "done"
}

func main() {
	http.Handle("/", templ.Handler(views.Hello("Trevor")))

	port := ":3000"
	fmt.Printf("Listening on %s\n", port)
	http.ListenAndServe(port, nil)
}
