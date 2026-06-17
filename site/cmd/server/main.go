package webserver

import (
	"fmt"
	"net/http"

	"webserver/views"

	"github.com/a-h/templ"
)

func main() {
	http.Handle("/", templ.Handler(views.Hello("Trevor")))

	port := ":3000"
	fmt.Printf("Listening on %s\n", port)
	http.ListenAndServe(port, nil)
}
