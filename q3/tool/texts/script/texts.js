var mailfolder = getMailFolder();
var path = mailfolder + "\\profiles\\texts.xml";

var doc = null;

var viewXsl = loadXml("xsl\\view.xsl");
var editXsl = loadXml("xsl\\edit.xsl");

var creatingElement = null;
var editingElement = null;

function load() {
	doc = new ActiveXObject("MSXML2.DOMDocument.4.0");
	doc.async = false;
	
	if (!doc.load(path))
		doc.appendChild(doc.createElement("texts"));
	
	update();
}

function save() {
	if (window.confirm("Are you sure to overwrite?"))
		doc.save(path);
}

function reload() {
	if (window.confirm("Are you sure to discard all changes?"))
		load();
}

function update() {
	var body = doc.transformNode(viewXsl);
	document.body.innerHTML = body;
}

function create() {
	var element = doc.createElement("text");
	doc.documentElement.appendChild(element);
	openEditor(element);
	creatingElement = element;
}

function edit(position) {
	var element = doc.selectSingleNode("texts/text[" + position + "]");
	openEditor(element);
	editingElement = element;
}

function remove(position) {
	var element = doc.selectSingleNode("texts/text[" + position + "]");
	element.parentNode.removeChild(element);
	update();
}

function up(position) {
	var element = doc.selectSingleNode("texts/text[" + position + "]");
	var before = doc.selectSingleNode("texts/text[" + (position - 1) + "]");
	if (before != null) {
		element.parentNode.insertBefore(element, before);
		update();
	}
}

function down(position) {
	var element = doc.selectSingleNode("texts/text[" + position + "]");
	var before = doc.selectSingleNode("texts/text[" + (position - 0 + 2) + "]");
	element.parentNode.insertBefore(element, before);
	update();
}

function openEditor(element) {
	document.body.innerHTML = element.transformNode(editXsl);
}

function edit_ok() {
	var form = document.editForm;
	var element = creatingElement != null ? creatingElement : editingElement;
	
	if (form.name.value.length == 0) {
		alert("Name must be specified.");
		return;
	}
	element.setAttribute("name", form.name.value);
	
	while (element.lastChild != null)
		element.removeChild(element.lastChild);
	element.appendChild(doc.createTextNode(form.body.value));
	
	creatingElement = null;
	editingElement = null;
	
	update();
}

function edit_cancel() {
	if (creatingElement != null)
		creatingElement.parentNode.removeChild(creatingElement);
	creatingElement = null;
	editingElement = null;
	
	update();
}
