FROM scratch

COPY back /server
ADD static/ /static/
ADD templates/ /templates/

CMD ["/server"]
