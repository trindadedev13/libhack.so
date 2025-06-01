### SHOW WINDOW

# CHANGE Lcom/minimine/ TO YOUR APP PACKAGE NAME
# it calls the Native#showWindow(Landroid/content/Context)
invoke-static {p0}, Lcom/hack/Hack;->showWindow(Landroid/content/Context;)V

### END SHOW WINDOW

### GENERATE CHUNK OVERRIDE

invoke-static {p0, p1, p2}, Lcom/hack/Hack;->gerarChunk(Lcom/minimine/engine/Mundo;II)[[[Lcom/minimine/engine/Bloco;

move-result-object p1

return-object p1

### END GENERATE CHUNK OVERRIDE