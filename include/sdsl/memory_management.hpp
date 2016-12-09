// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file memory_management.hpp
\brief memory_management.hpp contains two function for allocating and deallocating memory
\author Simon Gog
*/
#ifndef INCLUDED_SDSL_MEMORY_MANAGEMENT
#define INCLUDED_SDSL_MEMORY_MANAGEMENT

#include "uintx_t.hpp"
#include "config.hpp"
#include "bits.hpp"
#include "memory_tracking.hpp"
#include "ram_fs.hpp"
#include "hugepage_allocator.hpp"

#include <chrono>
#include <algorithm>

namespace sdsl {


class memory_manager {
private:
	bool hugepages = false;

private:
	static memory_manager& the_manager()
	{
		static memory_manager m;
		return m;
	}

public:
	static uint64_t* alloc_mem(size_t size_in_bytes)
	{
#ifndef MSVC_COMPILER
		auto& m = the_manager();
		if (m.hugepages) {
			return (uint64_t*)hugepage_allocator::the_allocator().mm_alloc(size_in_bytes);
		}
#endif
		return (uint64_t*)calloc(size_in_bytes, 1);
	}
	static void free_mem(uint64_t* ptr)
	{
#ifndef MSVC_COMPILER
		auto& m = the_manager();
		if (m.hugepages and hugepage_allocator::the_allocator().in_address_space(ptr)) {
			hugepage_allocator::the_allocator().mm_free(ptr);
			return;
		}
#endif
		std::free(ptr);
	}
	static uint64_t* realloc_mem(uint64_t* ptr, size_t size)
	{
#ifndef MSVC_COMPILER
		auto& m = the_manager();
		if (m.hugepages and hugepage_allocator::the_allocator().in_address_space(ptr)) {
			return (uint64_t*)hugepage_allocator::the_allocator().mm_realloc(ptr, size);
		}
#endif
		return (uint64_t*)realloc(ptr, size);
	}

public:
	static void use_hugepages(size_t bytes = 0)
	{
#ifndef MSVC_COMPILER
		auto& m = the_manager();
		hugepage_allocator::the_allocator().init(bytes);
		m.hugepages = true;
#else
		throw std::runtime_error("hugepages not support on MSVC_COMPILER");
#endif
	}
	template <class t_vec>
	static void resize(t_vec& v, const typename t_vec::size_type size)
	{
		uint64_t old_size_in_bytes = ((v.m_size + 63) >> 6) << 3;
		uint64_t new_size_in_bytes = ((size + 63) >> 6) << 3;
		bool	 do_realloc		   = old_size_in_bytes != new_size_in_bytes;
		v.m_size				   = size;
		if (do_realloc || v.m_data == nullptr) {
			// Note that we allocate 8 additional bytes if m_size % 64 == 0.
			// We need this padding since rank data structures do a memory
			// access to this padding to answer rank(size()) if size()%64 ==0.
			// Note that this padding is not counted in the serialize method!
			size_t allocated_bytes = (size_t)(((size + 64) >> 6) << 3);
			v.m_data			   = memory_manager::realloc_mem(v.m_data, allocated_bytes);
			if (allocated_bytes != 0 && v.m_data == nullptr) {
				throw std::bad_alloc();
			}
			// update and fill with 0s
			if (v.bit_size() < v.capacity()) {
				uint8_t len			   = (uint8_t)(v.capacity() - v.bit_size());
				uint8_t in_word_offset = (uint8_t)(v.bit_size() & 0x3F);
				bits::write_int(v.m_data + (v.bit_size() >> 6), 0, in_word_offset, len);
			}
			if (((v.m_size) % 64) == 0) { // initialize unreachable bits with 0
				v.m_data[v.m_size / 64] = 0;
			}

			// update stats
			if (do_realloc) {
				memory_monitor::record((int64_t)new_size_in_bytes - (int64_t)old_size_in_bytes);
			}
		}
	}
	template <class t_vec>
	static void clear(t_vec& v)
	{
		int64_t size_in_bytes = ((v.m_size + 63) >> 6) << 3;
		// remove mem
		memory_manager::free_mem(v.m_data);
		v.m_data = nullptr;

		// update stats
		if (size_in_bytes) {
			memory_monitor::record(size_in_bytes * -1);
		}
	}

	static int open_file_for_mmap(std::string& filename, std::ios_base::openmode mode)
	{
		if (is_ram_file(filename)) {
			return ram_fs::open(filename);
		}
#ifdef MSVC_COMPILER
		int fd = -1;
		if (!(mode & std::ios_base::out))
			_sopen_s(&fd, filename.c_str(), _O_BINARY | _O_RDONLY, _SH_DENYNO, _S_IREAD);
		else
			_sopen_s(&fd, filename.c_str(), _O_BINARY | _O_RDWR, _SH_DENYNO, _S_IREAD | _S_IWRITE);
		return fd;
#else
		if (!(mode & std::ios_base::out))
			return open(filename.c_str(), O_RDONLY);
		else
			return open(filename.c_str(), O_RDWR);
#endif
		return -1;
	}

	static void* mmap_file(int fd, uint64_t file_size, std::ios_base::openmode mode)
	{
		if (file_size == 0) {
			std::cout << "file_size=0" << std::endl;
			return nullptr;
		}
		if (is_ram_file(fd)) {
			if (ram_fs::file_size(fd) < file_size) return nullptr;
			auto& file_content = ram_fs::content(fd);
			return file_content.data();
		}
		memory_monitor::record(file_size);
#ifdef MSVC_COMPILER
		HANDLE fh = (HANDLE)_get_osfhandle(fd);
		if (fh == INVALID_HANDLE_VALUE) {
			return nullptr;
		}
		HANDLE fm;
		if (!(mode & std::ios_base::out)) { // read only?
			fm = CreateFileMapping(fh, NULL, PAGE_READONLY, 0, 0, NULL);
		} else
			fm = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (fm == NULL) {
			return nullptr;
		}
		void* map = nullptr;
		if (!(mode & std::ios_base::out)) { // read only?
			map = MapViewOfFile(fm, FILE_MAP_READ, 0, 0, file_size);
		} else
			map = MapViewOfFile(fm, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, file_size);
		// we can close the file handle before we unmap the view: (see UnmapViewOfFile Doc)
		// Although an application may close the file handle used to create a file mapping object,
		// the system holds the corresponding file open until the last view of the file is unmapped.
		// Files for which the last view has not yet been unmapped are held open with no sharing restrictions.
		CloseHandle(fm);
		return map;
#else
		void* map = nullptr;
		if (!(mode & std::ios_base::out))
			map = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
		else
			map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (map == MAP_FAILED) map = nullptr; // unify windows and unix error behaviour
		return map;
#endif
		return nullptr;
	}

	static int mem_unmap(int fd, void* addr, const uint64_t size)
	{
		if (addr == nullptr) {
			return 0;
		}
		if (is_ram_file(fd)) {
			return 0;
		}
		memory_monitor::record(-((int64_t)size));
#ifdef MSVC_COMPILER
		if (UnmapViewOfFile(addr)) return 0;
		return -1;
#else
		return munmap(addr, size);
#endif
		return -1;
	}

	static int close_file_for_mmap(int fd)
	{
		if (is_ram_file(fd)) {
			return ram_fs::close(fd);
		}
#ifdef MSVC_COMPILER
		return _close(fd);
#else
		return close(fd);
#endif
		return -1;
	}

	static int truncate_file_mmap(int fd, const uint64_t new_size)
	{
		if (is_ram_file(fd)) {
			return ram_fs::truncate(fd, new_size);
		}
#ifdef MSVC_COMPILER
		auto ret		  = _chsize_s(fd, new_size);
		if (ret != 0) ret = -1;
		return ret;
#else
		return ftruncate(fd, new_size);
#endif
		return -1;
	}
};

inline void
output_event_json(std::ostream& out, const memory_monitor::mm_event& ev, const memory_monitor& m)
{
	out << "\t\t"
		<< "\"name\" : "
		<< "\"" << ev.name << "\",\n";
	out << "\t\t"
		<< "\"usage\" : ["
		<< "\n";
	for (size_t j = 0; j < ev.allocations.size(); j++) {
		out << "\t\t\t["
			<< std::chrono::duration_cast<std::chrono::milliseconds>(ev.allocations[j].timestamp -
																	 m.start_log)
			   .count()
			<< "," << ev.allocations[j].usage << "]";
		if (j + 1 < ev.allocations.size()) {
			out << ",\n";
		} else {
			out << "\n";
		}
	}
	out << "\t\t"
		<< "]\n";
}

template <>
inline void write_mem_log<JSON_FORMAT>(std::ostream& out, const memory_monitor& m)
{
	auto events = m.completed_events;
	std::sort(events.begin(), events.end());

	// output
	out << "[\n";
	for (size_t i = 0; i < events.size(); i++) {
		out << "\t{\n";
		output_event_json(out, events[i], m);
		if (i < events.size() - 1) {
			out << "\t},\n";
		} else {
			out << "\t}\n";
		}
	}
	out << "]\n";
}


inline std::string create_mem_html_header()
{
	std::stringstream jsonheader;
	jsonheader << "<html>\n"
			   << "<head>\n"
			   << "<meta charset=\"utf-8\">\n"
			   << "<style>\n"
			   << "    body { font: 11px sans-serif; }\n"
			   << "    .rule { height: 90%; position: absolute; border-right: 1px dotted #000; "
				  "text-align: right; }\n"
			   << "</style>\n"
			   << "<title>sdsl memory usage visualization</title>\n"
			   << "<script src=\"http://d3js.org/d3.v3.js\"></script>\n"
			   << "</head>\n"
			   << "<body marginwidth=\"0\" marginheight=\"0\">\n"
			   << "<button><a id=\"download\">Save as SVG</a></button>\n"
			   << "<div class=\"chart\"><div id=\"visualization\"></div></div><script>\n";
	return jsonheader.str();
}

inline std::string create_mem_js_body(const std::string& jsonObject)
{
	std::stringstream jsonbody;
	jsonbody
	<< "var events = " << jsonObject << ";\n"
	<< "var w = window,d = document,e = d.documentElement,g = d.getElementsByTagName('body')[0],\n"
	<< "  xw = w.innerWidth || e.clientWidth || g.clientWidth,\n"
	<< "  yh = w.innerHeight || e.clientHeight || g.clientHeight;\n\n"
	<< "var margin = {top: 20,right: 80,bottom: 120,left: 120},\n"
	<< "  width = xw - margin.left - margin.right,height = yh - margin.top - margin.bottom;\n"
	<< "var x = d3.scale.linear().range([0, width]);\n"
	<< "var y = d3.scale.linear().range([height, 0]);\n"
	<< "var xAxis = d3.svg.axis().scale(x).orient(\"bottom\");\n"
	<< "var yAxis = d3.svg.axis().scale(y).orient(\"left\").ticks(5);\n"
	<< "var color = d3.scale.category10();\n"
	<< "var x_max = d3.max(events, function (d) { return d3.max(d.usage, function (u) { return "
	   "u[0] / 1000;})})\n"
	<< "var y_max = d3.max(events, function (d) { return d3.max(d.usage, function (u) { return 1.1 "
	   "* u[1] / (1024 * 1024);})})\n"
	<< "var peak = d3.max(events, function (d) { return d3.max(d.usage, function (u) { return "
	   "u[1]; })})\n"
	<< "var data = []\nevents.forEach(function (d) { data = data.concat(d.usage); });\n"
	<< "var peakelem = data.filter(function (a) { return a[1] == peak; });\n"
	<< "var peakelem = peakelem.splice(0,1);\n"
	<< "x.domain([0, x_max]);\n y.domain([0, y_max]);\n"
	<< "var svg = d3.select(\"#visualization\").append(\"svg\")\n"
	<< "  .attr(\"width\", width + margin.left + margin.right)\n"
	<< "  .attr(\"height\", height + margin.top + margin.bottom)\n"
	<< "  .attr(\"xmlns\", \"http://www.w3.org/2000/svg\")\n"
	<< "  .append(\"g\").attr(\"transform\",\"translate(\" + margin.left + \",\" + margin.top + "
	   "\")\");\n\n"
	<< "  svg.append(\"g\").attr(\"class\", \"xaxis\").attr(\"transform\", \"translate(0,\" + "
	   "height + \")\")\n"
	<< "  .call(xAxis).append(\"text\").attr(\"text-anchor\", \"end\")\n"
	<< "  .attr(\"shape-rendering\", \"crispEdges\").attr(\"x\", width / 2 + 50).attr(\"y\", "
	   "70).attr(\"shape-rendering\", \"crispEdges\")\n"
	<< "  .attr(\"font-family\", \"sans-serif\").attr(\"font-size\", \"20px\").text(\"Time "
	   "(seconds)\");\n\n"
	<< "svg.append(\"g\").attr(\"class\", "
	   "\"yaxis\").call(yAxis).append(\"text\").attr(\"transform\", \"rotate(-90)\").attr(\"x\", "
	   "-height / 2 + 50)\n"
	<< "  .attr(\"y\", -80).attr(\"shape-rendering\", \"crispEdges\").attr(\"font-family\", "
	   "\"sans-serif\").attr(\"font-size\", \"20px\").style(\"text-anchor\", \"end\")\n"
	<< "  .text(\"Memory Usage (MiB)\");\n\n"
	<< "svg.selectAll(\".tick text\").style(\"font-size\", \"20px\");\n"
	<< "svg.selectAll(\".xaxis .tick text\").attr(\"dy\", 23);\nsvg.selectAll(\".yaxis .tick "
	   "text\").attr(\"dx\", -10);\n"
	<< "svg.selectAll(\"line\").attr(\"fill\", \"none\").attr(\"stroke\", "
	   "\"black\")\nsvg.selectAll(\"path\").attr(\"fill\", \"none\").attr(\"stroke\", "
	   "\"black\")\n\n"
	<< "svg.selectAll(\"line.horizontalGrid\").data(y.ticks(5)).enter().append(\"line\")\n"
	<< "  .attr({\"class\": \"horizontalGrid\",\"x1\": 0,\"x2\": width,\"y1\": function (d) { "
	   "return y(d);},\n"
	<< "     \"y2\": function (d) { return y(d); }, \"fill\": \"none\", \"shape-rendering\": "
	   "\"crispEdges\",\n"
	<< "     \"stroke\": \"lightgrey\",\"stroke-dasharray\": \"10,10\",\"stroke-width\": "
	   "\"1.5px\"});\n\n"
	<< "var area = d3.svg.area().x(function (d) { return x(d[0] / 1000);}).y0(height).y1(function "
	   "(d) { return y(d[1] / (1024 * 1024))});\n\n"
	<< "var ev = "
	   "svg.selectAll(\".event\").data(events).enter().append(\"svg:path\").attr(\"class\", "
	   "\"area\")\n"
	<< "  .attr(\"fill\", function (d) { return d3.rgb(color(d.name)); })\n"
	<< "  .attr(\"d\", function (d) { return area(d.usage) })\n"
	<< "  .style(\"stroke\", function (d) { return "
	   "d3.rgb(color(d.name)).darker(2);}).style(\"stroke-width\", \"2px\")\n\n"
	<< "svg.selectAll(\".dot\").data(peakelem).enter().append(\"circle\").attr(\"r\", "
	   "3).attr(\"fill\", \"red\")\n"
	<< "  .attr(\"cx\", function (d) {return x(d[0] / 1000)})\n"
	<< "  .attr(\"cy\", function (d) {return y(d[1] / (1024 * 1024))})\n"
	<< "  .attr(\"fill\", \"red\").attr(\"stroke-width\", 2).attr(\"stroke\", \"#cc0000\")\n\n"
	<< "svg.selectAll(\".dot\").data(peakelem).enter().append(\"svg:text\")\n"
	<< "  .attr(\"x\", function (d) {return x(d[0] / 1000)}).attr(\"y\", function (d) {return "
	   "y(d[1] / (1024 * 1024) * 1.025)})\n"
	<< "  .text(function (d) {return \"Peak Usage: \" + Math.round(d[1] / (1024 * 1024)) + \" "
	   "MB\"})\n"
	<< "  .attr(\"font-size\", 12).attr(\"fill\", \"red\");\n\n"
	<< "svg.selectAll(\".dot\").data(peakelem).enter().append(\"circle\")\n"
	<< "  .attr(\"r\", 5).attr(\"fill\", \"red\")\n"
	<< "  .attr(\"cx\", function (d) {return x(d[0] / 1000)})\n"
	<< "  .attr(\"cy\", function (d) {return y(d[1] / (1024 * 1024))})\n"
	<< "  .attr(\"fill\", \"none\").attr(\"stroke-width\", 2).attr(\"stroke\", "
	   "\"#cc0000\").each(pulsepeak());\n\n"
	<< "function pulsepeak() { return function (d, i, j) {\n"
	<< "  d3.select(this).attr(\"r\", 5).style(\"stroke-opacity\", 1.0).transition()\n"
	<< "    .ease(\"linear\").duration(1000).attr(\"r\", 10).style(\"stroke-opacity\", "
	   "0.0).each(\"end\", pulsepeak());};}\n\n"
	<< "var vertical = d3.select(\".chart\").append(\"div\").attr(\"class\", \"remove\")\n"
	<< "  .style(\"position\", \"absolute\").style(\"z-index\", \"19\").style(\"width\", \"1px\")\n"
	<< "  .style(\"height\", height - margin).style(\"top\", \"30px\").style(\"bottom\", "
	   "\"50px\")\n"
	<< "  .style(\"left\", \"0px\").style(\"opacity\", \"0.4\").style(\"background\", "
	   "\"black\");\n\n"
	<< "var tooltip = d3.select(\".chart\").append(\"div\").attr(\"class\", \"remove\")\n"
	<< "  .style(\"position\", \"absolute\").style(\"z-index\", \"20\").style(\"visibility\", "
	   "\"hidden\").style(\"top\", \"10px\");\n\n"
	<< "var circle = svg.append(\"circle\").attr(\"cx\", 100).attr(\"cy\", 350).attr(\"r\", "
	   "3).attr(\"fill\", \"black\").style(\"opacity\", \"0\")\n\n"
	<< "d3.select(\"svg\").on(\"mousemove\", function () {\n"
	<< "  mousex = d3.mouse(this);\n"
	<< "  if (mousex[0] < margin.left + 3 || mousex[0] >= xw - margin.right) {\n"
	<< "    vertical.style(\"opacity\", \"0\"); tooltip.style(\"opacity\", \"0\"); "
	   "circle.style(\"opacity\", \"0\")\n"
	<< "  } else {\n"
	<< "    var xvalue = x.invert(mousex[0] - margin.left); var pos = findPosition(xvalue)\n"
	<< "    vertical.style(\"opacity\", \"0.4\"); tooltip.style(\"opacity\", \"1\"); "
	   "circle.style(\"opacity\", \"1\")\n"
	<< "    circle.attr(\"cx\", pos.x).attr(\"cy\", pos.y); vertical.style(\"left\", mousex[0] + "
	   "\"px\");tooltip.style(\"left\", mousex[0] + 15 + \"px\")\n"
	<< "    tooltip.html(\"<p>\" + xvalue.toFixed(2) + \" Seconds <br>\" + Math.round(pos.mem) + "
	   "\" MiB <br> \" + pos.name + "
	<< "  \"<br> Phase Time: \" + pos.ptime + \" Seconds </p>\").style(\"visibility\", "
	   "\"visible\");\n"
	<< "  }\n})"
	<< ".on(\"mouseover\", function () {\n"
	<< "  mousex = d3.mouse(this);\n  if (mousex[0] < margin.left + 3 || mousex[0] > xw - "
	   "margin.right) {\n"
	<< "    vertical.style(\"opacity\", \"0\")\n  } else {\n    vertical.style(\"opacity\", "
	   "\"0.4\");vertical.style(\"left\", mousex[0] + 7 + \"px\")\n}})\n"
	<< "d3.select(\"#download\").on(\"click\", function () {\n"
	<< "d3.select(this).attr(\"href\", 'data:application/octet-stream;base64,' + "
	   "btoa(d3.select(\"#visualization\").html())).attr(\"download\", \"viz.svg\")})\n\n"
	<< "function "
	   "findPosition(e){correctArea=d3.selectAll(\".area\").filter(function(t){if(t.usage[0][0]<=e*"
	   "1e3&&t.usage[t.usage.length-1][0]>=e*1e3){return true}"
	<< "return false});if(correctArea.empty()){return 0}var t=new "
	   "Array;correctArea[0].forEach(function(n){t.push(findYValueinArea(n,e))});"
	<< "max_elem=d3.max(t,function(e){return e.mem});var n=t.filter(function(e){return "
	   "e.mem==max_elem});return n[0]}"
	<< "function findYValueinArea(e,t){len=e.getTotalLength();var n=0;var r=len;for(var "
	   "i=0;i<=len;i+=50){var s=e.getPointAtLength(i);"
	<< "var o=x.invert(s.x);var u=y.invert(s.y);if(u>0&&o>t){n=Math.max(0,i-50);r=i;break}}var "
	   "a=e.getPointAtLength(0);"
	<< "var f=1;while(n<r){var "
	   "l=(r+n)/"
	   "2;a=e.getPointAtLength(l);target_x=x.invert(a.x);if((l==n||l==r)&&Math.abs(target_x-t)>.01)"
	   "{break}if(target_x>t)r=l;"
	<< "else if(target_x<t)n=l;else{break}if(f>50){break}f++}var c=new "
	   "function(){this.mem=y.invert(a.y);this.name=e.__data__.name;"
	<< "this.min=d3.min(e.__data__.usage,function(e){return "
	   "e[0]/1e3});this.max=d3.max(e.__data__.usage,function(e){return e[0]/1e3});"
	<< "this.ptime=Math.round(this.max-this.min);this.x=a.x;this.y=a.y};return "
	   "c}\n</script></body></html>";
	return jsonbody.str();
}


template <>
inline void write_mem_log<HTML_FORMAT>(std::ostream& out, const memory_monitor& m)
{
	std::stringstream json_data;
	write_mem_log<JSON_FORMAT>(json_data, m);

	out << create_mem_html_header();
	out << create_mem_js_body(json_data.str());
}

} // end namespace

#endif
