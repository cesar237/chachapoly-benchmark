package main

import (
	"crypto/cipher"
	"crypto/rand"
	"fmt"
	"os"
	"strconv"
	"time"

	"golang.org/x/crypto/chacha20poly1305"
)

func RoutineEncrypt(id int, stats []int64, queue <-chan []byte, aead cipher.AEAD, nonce []byte, control <-chan int) {
	// msg := []byte("Gophers, gophers, gophers everywhere!")
	for {
		// var encryptedMsg []byte
		select {
		case <-control:
			return
		case msg := <-queue:
			// Encrypt the message and append the ciphertext to the nonce.
			aead.Seal(nonce, nonce, msg, nil)
			// fmt.Printf("%s\n", encryptedMsg)
			stats[id] += 1
		}
	}
}

func generate_message(msg []byte, queue chan<- []byte, batch int, delay_ms time.Duration, total_time time.Duration) {
	for timeout := time.After(total_time * time.Second); ; {
		select {
		case <-timeout:
			return
		default:
			for i := 0; i < batch; i++ {
				queue<- msg
			}
			time.Sleep(delay_ms * time.Millisecond)
		}
	}
}

// Decryption.
func RoutineDecrypt(id int, stats []int64, queue <- chan []byte, aead cipher.AEAD, control <-chan int) {

	for {
		select {
		case <-control:
			return
		case msg := <- queue:
			if len(msg) < aead.NonceSize() {
				return
			}

			// Split nonce and ciphertext.
			nonce, ciphertext := msg[:aead.NonceSize()], msg[aead.NonceSize():]

			// Decrypt the message and check it wasn't tampered with.
			_, err := aead.Open(nil, nonce, ciphertext, nil)
			if err != nil {
				panic(err)
			}
			// fmt.Printf("%s\n", plaintext)
			stats[id] += 1
		}
	}
}

func main() {
	// Usage:
	// chachapoly-bench encryption|decryption msg-len n-routines pps duration 
	// test := "encryption"
	if len(os.Args) != 6 {
		fmt.Println(
			"Usage: chachapoly-bench encryption|decryption msg-len n-routines pps duration")
		os.Exit(1)
	}
	test := os.Args[1]
	msg_len, err := strconv.Atoi(os.Args[2])
	if err != nil {
		fmt.Println("msg_len is not a int")
		fmt.Println(
			"Usage: chachapoly-bench encryption|decryption msg-len n-routines pps duration")
		os.Exit(1)
	}
	n_routines, err := strconv.Atoi(os.Args[3])
	if err != nil {
		fmt.Println("n-routines is not a int")
		fmt.Println(
			"Usage: chachapoly-bench encryption|decryption msg-len n-routines pps duration")
		os.Exit(1)
	}
	pps, err := strconv.Atoi(os.Args[4])
	if err != nil {
		fmt.Println("pps is not a int")
		fmt.Println(
			"Usage: chachapoly-bench encryption|decryption msg-len n-routines pps duration")
		os.Exit(1)
	}
	total_duration, err := strconv.Atoi(os.Args[5])
	if err != nil {
		fmt.Println("total-duration is not a int")
		fmt.Println(
			"Usage: chachapoly-bench encryption|decryption msg-len n-routines pps duration")
		os.Exit(1)
	}

	delay_ms := 1_000 / (pps/1000)
	stats := make([]int64, n_routines)
	control := make(chan int)
	queue := make(chan []byte, 10240)

	key := make([]byte, chacha20poly1305.KeySize)
	if _, err := rand.Read(key); err != nil {
		panic(err)
	}

	aead, err := chacha20poly1305.New(key)
	if err != nil {
		panic(err)
	}

	// Select a random nonce, and leave capacity for the ciphertext.
	nonce := make([]byte, chacha20poly1305.NonceSize, chacha20poly1305.NonceSize+msg_len+16)
	if _, err := rand.Read(nonce); err != nil {
		panic(err)
	}

	msg := make([]byte, msg_len)
	if _, err := rand.Read(msg); err != nil {
		panic(err)
	}
	encryptedMsg := aead.Seal(nonce, nonce, msg, nil)

	switch test {
	case "encryption":
		go generate_message(
			msg,
			queue,
			1000,
			time.Duration(delay_ms),
			time.Duration(total_duration),
		)
		for i := 0; i < n_routines ; i++ {
			go RoutineEncrypt(i, stats, queue, aead, nonce, control)
		}

	case "decryption":
		go generate_message(
			encryptedMsg,
			queue,
			1000,
			time.Duration(delay_ms),
			time.Duration(total_duration),
		)
		for i := 0; i < n_routines ; i++ {
			go RoutineDecrypt(i, stats, queue, aead, control)
		}
	
	default:
		fmt.Println("Error on the type of test to run")
	}

	time.Sleep(time.Duration(total_duration + 1) * time.Second)
	
	// To stop encryption or decryption routines
	for i := 0; i < n_routines; i++ {
		control <- 0
	}
	close(queue)

	var msg_processed int64 = 0
	for i := 0; i < n_routines; i++ {
		msg_processed += stats[i]
	}

	fmt.Printf("%s;%d;%d;%d;%d;%d;%.2f;%.2f\n",
		test,
		msg_len,
		n_routines,
		pps,
		total_duration,
		msg_processed,
		float64(msg_processed) / float64(total_duration),
		(float64(msg_processed*int64(msg_len))/1e6)/float64(total_duration),
	)
	// fmt.Printf("total_packets: %d\n", msg_processed)
	// fmt.Printf("throughput_pps: %.2f\n", float64(msg_processed) / float64(total_duration))
	// fmt.Printf("Throuhgput_mbps: %.2f\n", 
	// 	(float64(msg_processed*int64(msg_len))/1e6)/float64(total_duration))
}
