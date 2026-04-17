FROM scratch

COPY front /server

EXPOSE 8080

CMD ["/server"]
