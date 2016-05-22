#pragma once
#include "stdafx.h"
#include "engine.h"
#include "poly.h"
#include "tinyfiledialogs.h"
#include "json\json.h"
#include <iomanip>
#include <imgui/imgui.h>
#include "imgui/imconfig.h"
#include "imgui-backends/SFML/imgui-events-SFML.h"
#include "imgui-backends/SFML/imgui-rendering-SFML.h"
#include "imgui/imguicolorpicker.h"

// Fix VS warning about posix or something-or-other
#ifdef _WIN32
#define strdup _strdup
#endif

// The title of the window set in the constructor.
#define WINDOWTITLE "Lowpoly Editor"
// Starting window size.
#define WINDOW_X 640
#define WINDOW_Y 480
// Fixed framerate set in the engine constructor.
#define FRAMERATE 144
// Range in pixels to snap to already existing points.
#define GRABDIST 10  

// Background color 
#define BGCOLOR sf::Color(125,125,125,255)

// Number of decimals to care about when using randdbl.
// RPRECISION should equal 10^x where x is the relevant decimal count.
#define RPRECISION 10000
// Returns a random double between a and b.
#define randdbl(a,b) ((rand()%(RPRECISION*(b-a)))/(RPRECISION*((float)b-a))+a)

// Returns the distance between two vectors.
float v2fdistance(sf::Vector2f a, sf::Vector2f b){
	return std::sqrt((b.x - a.x)*(b.x - a.x) + (b.y - a.y)*(b.y - a.y));
}

// Constructor for the main engine.
// Sets up renderwindow variables and loads an image.
Engine::Engine(int aaLevel) {
	sf::ContextSettings settings;
	settings.antialiasingLevel = aaLevel;
	window = new sf::RenderWindow(sf::VideoMode(WINDOW_X, WINDOW_Y), WINDOWTITLE, sf::Style::Default,settings);
	window->setFramerateLimit(FRAMERATE);
	window->setVerticalSyncEnabled(false);
	window->setKeyRepeatEnabled(false); 
	if (load() != 0){
		std::exit(1);
	}
	view.reset(sf::FloatRect(0, 0, WINDOW_X, WINDOW_Y));
	window->setView(view);
	drawimg.setTexture(image);

	// Initialize GUI and backend
	ImGui::SFML::SetRenderTarget(*window);
	ImGui::SFML::InitImGuiRendering();
	ImGui::SFML::SetWindow(*window);
	ImGui::SFML::InitImGuiEvents();
}

// Destructor for the main engine.
// Deletes the window.
Engine::~Engine() {
	delete window;
}

// Main loop of the engine.
// Delegates events to the Engine::handleEvents() function,
// Saves on exit, 
// and delegates update/draw as well.
void Engine::run() {
	while (window->isOpen()) {
		sf::Event event;
		while (window->pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				saveJSON();
				ImGui::SFML::Shutdown();
				window->close();
				std::exit(1);
			}
			// Handle events in relation to the GUI
			handleGUItoggleEvent(event);
			// If the GUI is open pass events to it and block left clicks
			if (showColorPickerGUI || showImageExportGUI) {
				ImGui::SFML::ProcessEvent(event);
				if (!(event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)) {
					handleEvents(event);
				}
			}
			// If the GUI is closed, run all events regardless
			else {
				handleEvents(event);
			}
		}
		window->clear(BGCOLOR);
		// If a GUI is up update them
		if (showColorPickerGUI || showImageExportGUI) {
			ImGui::SFML::UpdateImGui();
			ImGui::SFML::UpdateImGuiRendering();
			if (showColorPickerGUI && !showImageExportGUI) {
				createColorPickerGUI();
			}
			else if (showImageExportGUI && !showColorPickerGUI) {
				createImageExportGUI();
			}
		}
		// Main loop
		update();
		draw();
		// Render UI
		if (showColorPickerGUI || showImageExportGUI) {
			ImGui::Render();
		}
		window->display();
	}
}

// Loads an image into the engine variables, 
// storing the names of the files to save into vfile and sfile.
// If the files do not exist, they are created.
int Engine::load(){
	const char* filter[3] = { "*.png", "*.jpg", "*.gif" };
    const char* filenamecc = tinyfd_openFileDialog("Select image: ", "./", 3, filter, NULL, 0);
	if (filenamecc == NULL){
		return 1;
	}
	std::string filename = filenamecc;
	// Strip extension
	size_t lastindex = filename.find_last_of(".");
	std::string filenoext = filename.substr(0, lastindex);
	// Add new extensions
	const std::string sfext = ".svg";
	const std::string vfext = ".vertices";
	vfile = filenoext + vfext;
	sfile = filenoext + sfext;
	// Open/create based on if the file exists
	std::fstream vstream;
	vstream.open(vfile, std::ios::in);
	if (!vstream){
		vstream.open(vfile, std::ios::out);
	}
	std::fstream sstream;
	sstream.open(sfile, std::ios::in);
	if (!sstream){
		sstream.open(sfile, std::ios::out);
	}
	// Fail if the images do not open correctly
	if (!(image.loadFromFile(filename))){
		return 1;
	}
	if (!(img.loadFromFile(filename))){
		return 1;
	}
	// Load JSOn containing points, etc.
	loadJSON();
	vstream.close();
	sstream.close();
	return 0;
}

// Check if the GUI is being toggled; pause other inputs if it is
void Engine::handleGUItoggleEvent(sf::Event event) {
	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::C) {
		if (showImageExportGUI) { showImageExportGUI = !showImageExportGUI; }
		showColorPickerGUI = !showColorPickerGUI;
	}
	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::E) {
		if (showColorPickerGUI) { showColorPickerGUI = !showColorPickerGUI; }
		showImageExportGUI = !showImageExportGUI;
	}
}


// Handles the events that the sfml window recieves.
// The following events are handled:
//     Single press events (and their releases),
//     Click events (and their releases),
//     Scrolling,
//     Resizing.
void Engine::handleEvents(sf::Event event){
	// On single press event
	if (event.type == sf::Event::KeyPressed) {
		std::string text;
        // Toggles image smoothing
		if (event.key.code == sf::Keyboard::Slash){
			smoothnessToggle();
			imgsmooth ? text = "on" : text = "off";
			std::cout << "Image smoothing " << text << " (Slash)\n";
		}
        // Wireframe toggle for polygons
		if (event.key.code == sf::Keyboard::W){
			wireframe = !wireframe;
			wireframe ? text = "on" : text = "off";
			std::cout << "Wireframe " << text << " (W)\n";
		}
        // Hides background
		if (event.key.code == sf::Keyboard::H){
			hideimage = !hideimage;
			hideimage ? text = "hidden" : text = "shown";
			std::cout << "Background " << text << " (H)\n";
		}
        if (event.key.code == sf::Keyboard::X){
            showcenters = !showcenters;
            showcenters ? text = "Showing" : text = "Hiding";
            std::cout << text << " centers. (X)\n";
        }
		if (event.key.code == sf::Keyboard::P){
			showrvectors = !showrvectors;
			showrvectors ? text = "Showing" : text = "Hiding";
			std::cout << text << " points. (P)\n";
		}
        // Clears current selection
		if (event.key.code == sf::Keyboard::Space){
			clearSelection();
            std::cout << "Clearing selection (Spacebar) \n";
		}
        // Saves the file as a set of a SVG and ".vertices" file
		if (event.key.code == sf::Keyboard::S){
			saveVector(file);
			saveJSON();
            std::cout << "Saving file (S) \n";
		}
        // Camera panning without mousewheelclick
		if (event.key.code == sf::Keyboard::LControl){
            std::cout << "Panning while button held (LControl)\n";
			sf::Vector2i pointi = sf::Mouse::getPosition(*window);
			sf::Vector2f point;
			point.x = (float)pointi.x;
			point.y = (float)pointi.y;
			point = windowToGlobalPos(point);
			vdraginitpt = point;
			vdragflag = true;
		}
        // Deletes selected points, polys
		if (event.key.code == sf::Keyboard::Delete){
            std::cout << "Deleting selection (Delete) \n";
			deleteSelection();
		}
        // Reaverage colors
		if (event.key.code == sf::Keyboard::A) {
            std::cout << "Re-averaging color in polygon (A) \n";
            if (spoly != NULL){
                spoly->fillcolor = sf::Color(avgClr(rpoints[spoly->s1], rpoints[spoly->s2], rpoints[spoly->s3], 10));
            } else {
                "Can't change color - no polygon selected (C) \n";
            }
        }
        // Get color at mouse
		if (event.key.code == sf::Keyboard::O) {
            std::cout << "Setting selected polygon color to color at mouse (O)\n";
            if (spoly != NULL){
                sf::Vector2f point = getMPosFloat();
                point = windowToGlobalPos(point);
				point = getClampedImgPoint(point);
                sf::Color color = img.getPixel(point.x, point.y);
                spoly->fillcolor = color;
            } else {
                "Can't change color - no polygon selected (C) \n";
            }
        }
		// Send overlapping element to end of list
		if (event.key.code == sf::Keyboard::Comma){
			std::cout << "Sending triangle to the back of the draw order (,)\n";
			if (spoly != NULL){
				for (unsigned i = 0; i < polygons.size(); i++){
					if (polygons[i].selected == true){
						polygons.insert(polygons.begin(), polygons[i]);
						polygons.erase(1 + polygons.begin() + i);
					}
				}
				clearSelection();
			}
		}
		// Send overlapping element to the beginning of the list
		if (event.key.code == sf::Keyboard::Period){
			std::cout << "Sending triangle to the front of the draw order (.)\n";
			if (spoly != NULL){
				for (unsigned i = 0; i < polygons.size(); i++){
					if (polygons[i].selected == true){
						polygons.push_back(polygons[i]);
						polygons.erase(polygons.begin() + i);
					}
				}
				clearSelection();
			}
		}
	}

	// On release event (used for disabling flags)
	if (event.type == sf::Event::KeyReleased){
		if (event.key.code == sf::Keyboard::LControl){
			vdragflag = false;
		}
	}

	// On click
	if (event.type == sf::Event::MouseButtonPressed) {
		sf::Vector2f click = sf::Vector2f((float)event.mouseButton.x, (float)event.mouseButton.y);
		sf::Vector2f point = windowToGlobalPos(click);
		if (event.mouseButton.button == sf::Mouse::Left) {
			onLeftClick(point);
		}
		if (event.mouseButton.button == sf::Mouse::Right){
			onRightClick(point);
		}
		if (event.mouseButton.button == sf::Mouse::Middle){
			onMiddleClick(point);
		}
	}

	// On click release (used for disabling flags)
	if (event.type == sf::Event::MouseButtonReleased){
		if (event.mouseButton.button == sf::Mouse::Left){
			dragflag = false;
		}
		if (event.mouseButton.button == sf::Mouse::Middle){
			vdragflag = false;
		}
	}

	// On scroll (used for zooming)
	if (event.type == sf::Event::MouseWheelScrolled){
		if (event.mouseWheelScroll.delta < 0){
			viewzoom *= 1.5;
			view.zoom(1.5);
			window->setView(view);
		}
		else if (event.mouseWheelScroll.delta > 0){
			viewzoom *= 0.75;
			view.zoom(0.75);
			window->setView(view);
		}
	}

	// On window resize
	if (event.type == sf::Event::Resized){
		view.reset(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height));
		viewzoom = 1;
		window->setView(view);
	}
}

// Handles logic directly before drawing.
// This function runs every frame.
void Engine::update(){
	// safeguard bandaid fix for spoly FIX ME
	for (Poly& polygon : polygons){
		polygon.updateCShape(viewzoom);
		polygon.updateCenter();
	}
	for (Point& point : rpoints){
		point.updateCShape(viewzoom);
		point.vector = getClampedImgPoint(point.vector);
	}
	for (auto& point : nspoints){
		point->selected = false;
	}
	for (auto& point : spoints){
		point->selected = true;
	}

	handleCamera();

	if (wireframe == true && (wireframe != wireframels)){
		for (Poly& polygon : polygons){
			polygon.isWireframe = true;
		}
		wireframels = wireframe;
	}
	else if (wireframe != true && (wireframe != wireframels)){
		for (Poly& polygon : polygons){
			polygon.isWireframe = false;
		}
		wireframels = wireframe;
	}
	if (dragflag){
		sf::Vector2f point = getMPosFloat();
		point = windowToGlobalPos(point);
		sf::Vector2f rpoint = rpoints[nindex].vector;
		rpoints[nindex].vector = (pdragoffset + point);
	}
	if (vdragflag){
		sf::Vector2f point = getMPosFloat();
		point = windowToGlobalPos(point);
		vdragoffset.x = (vdraginitpt.x - point.x);
		vdragoffset.y = (vdraginitpt.y - point.y);
		view.move(vdragoffset);
		window->setView(view);
	}
}

// Draws the objects to the screen.
// This function runs every frame, preceded by Engine::update().
void Engine::draw(){
	// Reset view incase other objects change it
	window->setView(view);
	if (!hideimage){
		window->draw(drawimg);
	}
	for (Poly polygon : polygons){
		if (polygon.selected == false){
			window->draw(polygon.cshape);
		}
	}
	if (showrvectors){
		for (Point& point : rpoints){
			window->draw(point.cshape);
		}
	}
	for (Poly polygon : polygons){
		if (polygon.selected == true){
			window->draw(polygon.cshape);
		}
	}
    if (showcenters){
        for (Poly polygon : polygons){
            sf::CircleShape cshape = sf::CircleShape(2*viewzoom);
            cshape.setPosition(polygon.center);
            cshape.setFillColor(sf::Color(255,255,255,127));
            window->draw(cshape);
        }
    }
}

// Create GUI elements for color picker
void Engine::createColorPickerGUI() {
	ImGui::Begin("Color Picker");
	bool isPolygonSelected = false;
	if (spoly != NULL){
		float spolycolor[3];
		spolycolor[0] = spoly->fillcolor.r / 255.0f;
		spolycolor[1] = spoly->fillcolor.g / 255.0f;
		spolycolor[2] = spoly->fillcolor.b / 255.0f;
		if (ColorPicker3(spolycolor)){
			spoly->fillcolor = sf::Color(spolycolor[0] * 255.0f, spolycolor[1] * 255.0f, spolycolor[2] * 255.0f, 255);
		}
	}
	else {
		ImGui::Text("No polygon selected.");
	}
	ImGui::End();
}

// Create GUI elements for export
void Engine::createImageExportGUI() {

}

// Checks input for arrow keys and +/- for camera movement
void Engine::handleCamera() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && window->hasFocus()) {
		view.move(sf::Vector2f(-2 * viewzoom, 0));
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && window->hasFocus()) {
		view.move(sf::Vector2f(2 * viewzoom, 0));
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && window->hasFocus()) {
		view.move(sf::Vector2f(0, -2 * viewzoom));
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && window->hasFocus()) {
		view.move(sf::Vector2f(0, 2 * viewzoom));
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Dash) && window->hasFocus()) {
		viewzoom *= 1.01f;
		view.zoom(1.01f);
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal) && window->hasFocus()) {
		viewzoom *= 0.99f;
		view.zoom(0.99f);
		window->setView(view);
	}
}

/*////////////////////////////////////////////////////////////////////////////
//// Input functions:
//// Callbacks from Engine::handleEvents(). 
*/////////////////////////////////////////////////////////////////////////////

// On spacebar
void Engine::clearSelection() {
	spoints.clear();
	nspoints.clear();
	spointsin.clear();
	spoly = NULL;
	for (unsigned n = 0; n < rpoints.size(); n++) {
		nspoints.push_back(&rpoints[n]);
	}
	for (unsigned i = 0; i < polygons.size(); i++) {
		polygons[i].selected = false;
	}
}

// On slash
void Engine::smoothnessToggle() {
	image.setSmooth(!imgsmooth);
	imgsmooth = !imgsmooth;
	drawimg.setTexture(image);
}

// On delete
void Engine::deleteSelection() {
	if (spointsin.size() == 0 && spoly == NULL) {}
	else {
		std::vector<int> polyIndices;
		std::vector<int> rpointsIndices;
		for (unsigned i = 0; i < spointsin.size(); i++) {
			//printf("spoints Indices: %d \n", spointsin[i]);
			//rpointsIndices.push_back(spointsin[i]);
		}
		for (unsigned i = 0; i < polygons.size(); i++) {
			if (polygons[i].selected == true) {
				polyIndices.push_back(i);
			}
		}
		for (unsigned i = 0; i < spointsin.size(); i++) {
			for (unsigned j = 0; j < polygons.size(); j++) {
				for (unsigned k = 0; k < 3; k++) {
					if (polygons[j].sa[k] == spointsin[i]) {
						/*printf("Polygon %d has point %d (%d,%d) overlapping with rpoints[%d](%d,%d)\n", 
							j, 
							k, 
							rpoints[polygons[j].sa[k]].vector.x, 
							rpoints[polygons[j].sa[k]].vector.y, 
							rpoints[spointsin[i]],
							rpoints[spointsin[i]].vector.x,
							rpoints[spointsin[i]].vector.y);
						polyIndices.push_back(j);*/
						polyIndices.push_back(j);
					}
				}
			}
			rpointsIndices.push_back(spointsin[i]);
		}
		// Sort vectors and erase duplicates
		std::sort(polyIndices.begin(), polyIndices.end());
		polyIndices.erase(std::unique(polyIndices.begin(), polyIndices.end()), polyIndices.end());
		std::sort(rpointsIndices.begin(), rpointsIndices.end());
		rpointsIndices.erase(std::unique(rpointsIndices.begin(), rpointsIndices.end()), rpointsIndices.end());
		// Reverse indices for easier deletion of elements
		std::reverse(rpointsIndices.begin(), rpointsIndices.end());
		std::reverse(polyIndices.begin(), polyIndices.end());
		/*for (Poly& polygon : polygons){
			cout << "Polygon point 1 x:" << polygon.p1->vector.x << endl;
		}
		for (Point& rpoint : rpoints){
			cout << "rpoint x:" << rpoint.vector.x << endl;
		}*/
		for (unsigned i = 0; i < polyIndices.size(); i++) {
			polygons.erase(polygons.begin() + polyIndices[i]);
		}
		for (unsigned i = 0; i < rpointsIndices.size(); i++) {
			rpoints.erase(rpoints.begin() + rpointsIndices[i]);
		}
		for (unsigned p = 0; p < polygons.size(); p++) { // For each polygon
			for (int i = 0; i < 3; i++) { // For every point inside a polygon
				int counter = 0;
				for (unsigned rp = 0; rp < rpointsIndices.size(); rp++) {
					if (polygons[p].sa[i] > rpointsIndices[rp]) {
						counter++;
					}
				}
				polygons[p].sa[i] -= counter;
			}
			polygons[p].updatePointsToArray();
		}

		polyIndices.clear();
		rpointsIndices.clear();
		clearSelection();
		for (Poly& poly : polygons){
			poly.updatePointers(rpoints);
		}
	}
}

// On left click
void Engine::onLeftClick(sf::Vector2f point) {
	for (Poly& polygon : polygons) {
		polygon.selected = false;
	}
	//std::cout << "Placing vertex at adjusted point " << point.x << ", " << point.y << "\n";
	// Test whether to make new point or not
	bool ispointnear = false;
	nindex = -1;
	for (unsigned i = 0; i < rpoints.size(); i++) {
		Point& rpoint = rpoints[i];
		sf::Vector2f rpv = rpoint.vector;
		int clickdist = GRABDIST;
        // Check if mouse is in snapping range; then set near appropriately
		if ((point.x - rpv.x < clickdist*viewzoom && point.x - rpv.x > -clickdist*viewzoom) &&
			(point.y - rpv.y < clickdist*viewzoom && point.y - rpv.y > -clickdist*viewzoom)) {
			ispointnear = true;
			nindex = i;
		}
	}
	// If its near another, snap to it -> shared edges
	if (ispointnear) {
		//spoint = &rpoints[nindex]; // spoint is used to get desired mouse position
		sf::Vector2f mpos = getMPosFloat();
		mpos = windowToGlobalPos(mpos);
		// Init values for dragging, used above
		pdraginitpt = mpos;
		pdragoffset.x = rpoints[nindex].vector.x - mpos.x;
		pdragoffset.y = rpoints[nindex].vector.y - mpos.y;
		dragflag = true; // When dragflag is true then dragging occurs
						 // Set mouse position to middle of desired selected point
						 // This fixes mouse clicks moving points on accident
		// Check for snapping the same point twice for a new poly and catch it
		bool exists = false;
		for (unsigned i = 0; i < spointsin.size(); i++) {
			if (spointsin[i] == nindex) {
				exists = true;
			}
		}
		// If they didn't click the same point twice
		if (!exists) {
			spointsin.push_back(nindex);
			//std::cout << spoint;
			spoints.push_back(&rpoints[nindex]);
			if (spoints.size() == 3) {
				polygons.push_back(Poly(spoints[0], spoints[1], spoints[2],
					spointsin[spointsin.size() - 3],
					spointsin[spointsin.size() - 2],
					spointsin[spointsin.size() - 1],
					sf::Color::Green));
				int offset = polygons.size() - 1;
				polygons[offset].fillcolor = avgClr(rpoints[polygons[offset].s1], rpoints[polygons[offset].s2], rpoints[polygons[offset].s3], 10);
				clearSelection();
			}
		}
	}
	// Create a new point
	if (!ispointnear) {
		rpoints.push_back(Point(point, 5));
		spointsin.push_back(rpoints.size() - 1);
		spoint = NULL;
		spoints.push_back(&(rpoints[rpoints.size() - 1]));
		if (spoints.size() == 3) {
			// Create new polygon at last 3 points, assign it starting index
			// so that it can find itself in the vector after memory updates
			polygons.push_back(Poly(spoints[0], spoints[1], spoints[2],
				spointsin[spointsin.size() - 3],
				spointsin[spointsin.size() - 2],
				spointsin[spointsin.size() - 1],
				sf::Color::Green));
			int offset = polygons.size() - 1;
			polygons[offset].fillcolor = avgClr(rpoints[polygons[offset].s1], rpoints[polygons[offset].s2], rpoints[polygons[offset].s3], 10);
			clearSelection();
		}
	}
	// Only update polygon point-pointers on click
	// because the memory is only moved on click

    // Commented lines are debug prints for polygon pointers

	for (Poly& p : polygons) {
		p.updatePointers(rpoints);
	}
}

// On right click
void Engine::onRightClick(sf::Vector2f point) {
	if (polygons.size() > 0) {
		clearSelection();
		float dist = 100000000.0f;
		int pindex = -1;
		for (unsigned p = 0; p < polygons.size(); p++) {
			float cdist = v2fdistance(point, polygons[p].center);
			if (cdist < dist) {
				dist = cdist;
				pindex = p;
			}
		}
		polygons[pindex].selected = true;
		spoly = &polygons[pindex];
	}
}

// On middle wheel click
void Engine::onMiddleClick(sf::Vector2f point) {
	vdragflag = true;
	vdragoffset = sf::Vector2f(0, 0);
	vdraginitpt = point;
}

// On C
sf::Color Engine::chooseColor() {
	unsigned int coloruint;
	if (spoly != NULL){
		unsigned char unused_but_passed[8]; // im not sure what this is for
		sf::Color ccolor = spoly->fillcolor;
		int ccolorr = ccolor.r;
		int ccolorg = ccolor.g;
		int ccolorb = ccolor.b;
		std::stringstream sstrm;
		sstrm << "#" << std::hex << ccolorr << ccolorg << ccolorb;
		std::string ccolors = sstrm.str();
		const char* result = tinyfd_colorChooser("Choose color:", ccolors.c_str(), NULL, unused_but_passed);
		if (result == NULL){
			return ccolor;
		}
		char* cc = strdup(result);
		// strip hashtag
		for (int i = 0; i < 6; i++){
			cc[i] = cc[i + 1];
		}
		std::stringstream hex;
		int clr[3];
		for (int i = 0; i < 6; i += 2){
			hex << cc[i] << cc[i + 1];
			hex >> std::hex >> clr[i/2];
			hex.clear();
		}
		// sfml source:  return (r << 24) | (g << 16) | (b << 8) | a;
		coloruint = (clr[0] << 24) | (clr[1] << 16) | (clr[2] << 8) | 0xFF;
	}
	else {
		coloruint = 0;
	}
	return sf::Color(coloruint);
}

/*/////////////////////////////////////////////////////////////////////////////
//// Utility functions
*//////////////////////////////////////////////////////////////////////////////

// Returns the average color from the area between 3 points.
// Due to speed, it uses random samples.
sf::Color Engine::avgClr(Point& p1, Point& p2, Point& p3, int samples){
	int r = 0;
	int g = 0;
	int b = 0;
	for (int i = 0; i < samples; i++){
		sf::Vector2f pixel = randPt(p1, p2, p3);
		sf::Color color = img.getPixel(pixel.x, pixel.y);
		r += color.r;
		g += color.g;
		b += color.b;
	}
	r /= samples;
	g /= samples;
	b /= samples;
	return sf::Color(r, g, b, 255);
}

// Return a random point inside 3 points.
sf::Vector2f Engine::randPt(Point& p1, Point& p2, Point& p3){
	sf::Vector2f point;
	double r1 = randdbl(0, 1);
	double r2 = randdbl(0, 1);
	point.x = (1 - sqrt(r1)) * p1.vector.x + (sqrt(r1) * (1 - r2)) * p2.vector.x + (sqrt(r1) * r2) * p3.vector.x;
	point.y = (1 - sqrt(r1)) * p1.vector.y + (sqrt(r1) * (1 - r2)) * p2.vector.y + (sqrt(r1) * r2) * p3.vector.y;
	return point;
}

// Convert window (view) coordinates to global (real) coordinates.
sf::Vector2f Engine::windowToGlobalPos(const sf::Vector2f& vec) {
	sf::Vector2u winSize = window->getSize();
	sf::Vector2f center = view.getCenter();
	sf::Vector2f point = vec;
	point.x -= winSize.x / 2;
	point.y -= winSize.y / 2;
	point.x *= viewzoom;
	point.y *= viewzoom;
	point.x += center.x;
	point.y += center.y;
	return point;
};

// Convert global (real) coordiantes to window (view) coordinates.
sf::Vector2f Engine::globalToWindowPos(const sf::Vector2f& vec) {
	sf::Vector2u winSize = window->getSize();
	sf::Vector2f center = view.getCenter();
	sf::Vector2f point = vec;
	point.x -= center.x;
	point.y -= center.y;
	point.x /= viewzoom;
	point.y /= viewzoom;
	point.x += winSize.x / 2;
	point.y += winSize.y / 2;
	return point;
};

// Get the mouse position as a float.
sf::Vector2f Engine::getMPosFloat() {
	sf::Vector2i mposi = sf::Mouse::getPosition(*window);
	sf::Vector2f mpos;
	mpos.x = mposi.x;
	mpos.y = mposi.y;
	return mpos;
}

// Clamps a point to the image boundaries.
sf::Vector2f Engine::getClampedImgPoint(const sf::Vector2f& vec){
	sf::Vector2f result = vec;
	if (result.x < 0 || result.x > img.getSize().x){
		if (result.x < 0){
			result.x = 0;
		}
		else if (result.x > img.getSize().x) {
			result.x = img.getSize().x;
		}
	}
	if (result.y < 0 || result.y > img.getSize().y){
		if (result.y < 0){
			result.y = 0;
		}
		else if (result.y > img.getSize().y){
			result.y = img.getSize().y;
		}
	}
	return result;
}


/*////////////////////////////////////////////////////////////////////////////
//// Saving functions
*/////////////////////////////////////////////////////////////////////////////

// Saves the SVG of the image.
void Engine::saveVector(std::string filename){
	std::fstream sfilestrm;
	sfilestrm.open(sfile, std::ios::out | std::fstream::trunc);
	char headerc[350];
	const char *hdr = "<?xml version=\"1.0\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n<style type=\"text/css\"> polygon { stroke-width: .5; stroke-linejoin: round; } </style>";
	sprintf_s(headerc, hdr,
		image.getSize().x,
		image.getSize().y,
		image.getSize().x,
		image.getSize().y);
	std::string header = headerc;
	std::string footer = "\n</svg>";
	sfilestrm << header;
	for (unsigned i = 0; i < polygons.size(); i++){
		Poly& p = polygons[i];
		std::string pointslist = "";
		for (int j = 0; j < 3; j++){
			pointslist += std::to_string(rpoints[p.sa[j]].vector.x);
			pointslist += ",";
			pointslist += std::to_string(rpoints[p.sa[j]].vector.y);
			pointslist += " ";
		}
		std::string color = "rgb(";
		color += std::to_string(p.fillcolor.r);
		color += ",";
		color += std::to_string(p.fillcolor.g);
		color += ",";
		color += std::to_string(p.fillcolor.b);
		color += ")";
		std::string polygon = "";
		polygon += "<polygon style=\"fill:";
		polygon += color;
		polygon += ";stroke:";
		polygon += color;
		polygon += "\"";
		polygon += " points=\"";
		polygon += pointslist;
		polygon += "\"/>\n";
		sfilestrm << polygon;
	}
	sfilestrm << footer;
}

// Saves the JSON of the points, polygons, colors
void Engine::saveJSON(){
	// Clamp all points to bounds
	for (Point& point : rpoints){
		point.vector = getClampedImgPoint(point.vector);
	}
	Json::Value rootobj;
	for (unsigned i = 0; i < rpoints.size(); i++){
		rootobj["rpoints"][i]["vector"]["x"] = rpoints[i].vector.x;
		rootobj["rpoints"][i]["vector"]["y"] = rpoints[i].vector.y;
		rootobj["rpoints"][i]["size"] = rpoints[i].size;
		rootobj["rpoints"][i]["color"] = rpoints[i].color.toInteger();
	}
	for (unsigned i = 0; i < polygons.size(); i++){
		for (int j = 0; j < 3; j++){
			rootobj["polygons"][i]["pointindices"][j] = polygons[i].sa[j];
		}
		rootobj["polygons"][i]["color"] = polygons[i].fillcolor.toInteger();
	}
	std::fstream vfilestrm;
	vfilestrm.open(vfile, std::ios::out | std::ios::trunc);
	vfilestrm << rootobj << std::endl;
}

// Loads the JSON into the engine variables.
void Engine::loadJSON(){
	rpoints.clear();
	polygons.clear();
	std::fstream vfilestrm;
	vfilestrm.open(vfile, std::ios::in);
	Json::Value rootobj;
	if (vfilestrm.peek() == std::fstream::traits_type::eof()) {
		return;
	}
	vfilestrm >> rootobj;
	Json::Value jsonpolygons;
	Json::Value jsonrpoints;
	jsonpolygons = rootobj["polygons"];
	jsonrpoints = rootobj["rpoints"];
	for (unsigned i = 0; i < jsonrpoints.size(); i++){
		Point p;
		p.vector.x = rootobj["rpoints"][i]["vector"]["x"].asFloat();
		p.vector.y = rootobj["rpoints"][i]["vector"]["y"].asFloat();
		p.size = rootobj["rpoints"][i]["size"].asFloat();
		int c = rootobj["rpoints"][i]["color"].asInt64();
		p.color = sf::Color(c);
		rpoints.push_back(p);
	}
	for (unsigned i = 0; i < jsonpolygons.size(); i++){
		int ptl[3];
		for (int j = 0; j < 3; j++){
			ptl[j] = rootobj["polygons"][i]["pointindices"][j].asInt();
		}
		int c = rootobj["polygons"][i]["color"].asInt64();
		sf::Color color = sf::Color(c);
		Poly p = Poly(&rpoints[ptl[0]],
					  &rpoints[ptl[1]],
					  &rpoints[ptl[2]],ptl[0], ptl[1], ptl[2], color);
		p.updatePointsToArray();
		p.updateCenter();
		p.updateCShape(viewzoom);
		polygons.push_back(p);
	}
	std::cout << "total polygons loaded: " << polygons.size() << "\n";
}