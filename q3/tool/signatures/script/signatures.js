var fso = new ActiveXObject("Scripting.FileSystemObject");

var mailfolder = getMailFolder();
var path = mailfolder + "\\profiles\\signatures.xml";
var accounts = loadAccounts(mailfolder);

var doc = null;

var viewXsl = loadXml("xsl\\view.xsl");
var editXsl = loadXml("xsl\\edit.xsl");

var creatingElement = null;
var editingElement = null;

function load() {
	doc = new ActiveXObject("MSXML2.DOMDocument.4.0");
	doc.async = false;
	
	try {
		var file = fso.GetFile(path);
		doc.load(path);
	}
	catch (e) {
		doc.appendChild(doc.createElement("signatures"));
	}
	
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
	var element = doc.createElement("signature");
	doc.documentElement.appendChild(element);
	openEditor(element);
	creatingElement = element;
}

function edit(position) {
	var element = doc.selectSingleNode("signatures/signature[" + position + "]");
	openEditor(element);
	editingElement = element;
}

function remove(position) {
	var element = doc.selectSingleNode("signatures/signature[" + position + "]");
	element.parentNode.removeChild(element);
	update();
}

function openEditor(element) {
	var template = createXSLTemplate(editXsl);
	var processor = template.createProcessor();
	processor.addParameter("accounts", accounts);
	processor.input = element;
	processor.transform();
	document.body.innerHTML = processor.output;
}

function edit_ok() {
	var form = document.editForm;
	var element = creatingElement != null ? creatingElement : editingElement;
	
	if (form.name.value.length == 0) {
		alert("Name must be specified.");
		return;
	}
	element.setAttribute("name", form.name.value);
	
	var account = form.account.value;
	if (account == "__none__")
		element.removeAttribute("account");
	else if (account == "__regex__")
		element.setAttribute("account", form.account_regex.value);
	else
		element.setAttribute("account", account);
	
	if (form.isdefault.checked)
		element.setAttribute("default", "true");
	else
		element.removeAttribute("default");
	
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
