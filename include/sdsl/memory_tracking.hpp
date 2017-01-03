// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file memory_tracking.hpp
\brief memory_tracking.hpp contains two function for allocating and deallocating memory
\author Simon Gog
*/
#ifndef INCLUDED_SDSL_MEMORY_TRACKING
#define INCLUDED_SDSL_MEMORY_TRACKING

#include "uintx_t.hpp"
#include "config.hpp"
#include "bits.hpp"

#include <map>
#include <iostream>
#include <cstdlib>
#include <mutex>
#include <chrono>
#include <cstring>
#include <set>
#include <cstddef>
#include <stack>
#include <vector>
#include <atomic>
#include "config.hpp"
#include <fcntl.h>
#include <sstream>
#include <fstream>
#include <algorithm>


#ifdef MSVC_COMPILER
// windows.h has min/max macro which causes problems when using std::min/max
#define NOMINMAX
#include <windows.h>
#include <io.h>
#else
#include <sys/mman.h>
#include <unistd.h> // for getpid, file_size, clock_gettime
#endif

using namespace std::chrono;

namespace sdsl {

class spin_lock {
private:
	std::atomic_flag m_slock;

public:
	spin_lock() { m_slock.clear(); }
	void lock()
	{
		while (m_slock.test_and_set(std::memory_order_acquire)) {
			/* spin */
		}
	};
	void unlock() { m_slock.clear(std::memory_order_release); };
};


class memory_monitor;

template <format_type F>
void write_mem_log(std::ostream& out, const memory_monitor& m);

class memory_monitor {
public:
	using timer = std::chrono::high_resolution_clock;
	struct mm_alloc {
		timer::time_point timestamp;
		int64_t			  usage;
		mm_alloc(timer::time_point t, int64_t u) : timestamp(t), usage(u){};
	};
	struct mm_event {
		std::string			  name;
		std::vector<mm_alloc> allocations;
		mm_event(std::string n, int64_t usage) : name(n)
		{
			allocations.emplace_back(timer::now(), usage);
		};
		bool operator<(const mm_event& a) const
		{
			if (a.allocations.size() && this->allocations.size()) {
				if (this->allocations[0].timestamp == a.allocations[0].timestamp) {
					return this->allocations.back().timestamp < a.allocations.back().timestamp;
				} else {
					return this->allocations[0].timestamp < a.allocations[0].timestamp;
				}
			}
			return true;
		}
	};
	struct mm_event_proxy {
		bool			  add;
		timer::time_point created;
		mm_event_proxy(const std::string& name, int64_t usage, bool a) : add(a)
		{
			if (add) {
				auto& m = the_monitor();

				std::lock_guard<spin_lock> lock(m.spinlock);
				m.event_stack.emplace(name, usage);
			}
		}
		~mm_event_proxy()
		{
			if (add) {
				auto& m = the_monitor();

				std::lock_guard<spin_lock> lock(m.spinlock);
				auto&					   cur		= m.event_stack.top();
				auto					   cur_time = timer::now();
				cur.allocations.emplace_back(cur_time, m.current_usage);
				m.completed_events.emplace_back(std::move(cur));
				m.event_stack.pop();
				// add a point to the new "top" with the same memory
				// as before but just ahead in time
				if (!m.event_stack.empty()) {
					if (m.event_stack.top().allocations.size()) {
						auto last_usage = m.event_stack.top().allocations.back().usage;
						m.event_stack.top().allocations.emplace_back(cur_time, last_usage);
					}
				}
			}
		}
	};
	std::chrono::milliseconds log_granularity = std::chrono::milliseconds(20ULL);
	int64_t					  current_usage   = 0;
	bool					  track_usage	 = false;
	std::vector<mm_event>	 completed_events;
	std::stack<mm_event>	  event_stack;
	timer::time_point		  start_log;
	timer::time_point		  last_event;
	spin_lock				  spinlock;

private:
	// disable construction of the object
	memory_monitor(){};
	~memory_monitor()
	{
		if (track_usage) {
			stop();
		}
	}
	memory_monitor(const memory_monitor&) = delete;
	memory_monitor& operator=(const memory_monitor&) = delete;

private:
	static memory_monitor& the_monitor()
	{
		static memory_monitor m;
		return m;
	}

public:
	static void granularity(std::chrono::milliseconds ms)
	{
		auto& m			  = the_monitor();
		m.log_granularity = ms;
	}
	static int64_t peak()
	{
		auto&   m   = the_monitor();
		int64_t max = 0;
		for (auto events : m.completed_events) {
			for (auto alloc : events.allocations) {
				if (max < alloc.usage) {
					max = alloc.usage;
				}
			}
		}
		return max;
	}

	static void start()
	{
		auto& m		  = the_monitor();
		m.track_usage = true;
		// clear if there is something there
		if (m.completed_events.size()) {
			m.completed_events.clear();
		}
		while (m.event_stack.size()) {
			m.event_stack.pop();
		}
		m.start_log		= timer::now();
		m.current_usage = 0;
		m.last_event	= m.start_log;
		m.event_stack.emplace("unknown", 0);
	}
	static void stop()
	{
		auto& m = the_monitor();
		while (!m.event_stack.empty()) {
			m.completed_events.emplace_back(std::move(m.event_stack.top()));
			m.event_stack.pop();
		}
		m.track_usage = false;
	}
	static void record(int64_t delta)
	{
		auto& m = the_monitor();
		if (m.track_usage) {
			std::lock_guard<spin_lock> lock(m.spinlock);
			auto					   cur = timer::now();
			if (m.last_event + m.log_granularity < cur) {
				m.event_stack.top().allocations.emplace_back(cur, m.current_usage);
				m.current_usage = m.current_usage + delta;
				m.event_stack.top().allocations.emplace_back(cur, m.current_usage);
				m.last_event = cur;
			} else {
				if (m.event_stack.top().allocations.size()) {
					m.current_usage									 = m.current_usage + delta;
					m.event_stack.top().allocations.back().usage	 = m.current_usage;
					m.event_stack.top().allocations.back().timestamp = cur;
				}
			}
		}
	}
	static mm_event_proxy event(const std::string& name)
	{
		auto& m = the_monitor();
		if (m.track_usage) {
			return mm_event_proxy(name, m.current_usage, true);
		}
		return mm_event_proxy(name, m.current_usage, false);
	}
	template <format_type F>
	static void write_memory_log(std::ostream& out)
	{
		write_mem_log<F>(out, the_monitor());
	}
};

// minimal allocator from http://stackoverflow.com/a/21083096
template <typename T>
struct track_allocator {
	using value_type = T;

	track_allocator() = default;
	template <class U>
	track_allocator(const track_allocator<U>&)
	{
	}

	T* allocate(std::size_t n)
	{
		if (n <= std::numeric_limits<std::size_t>::max() / sizeof(T)) {
			size_t s = n * sizeof(T);
			if (auto ptr = std::malloc(s)) {
				memory_monitor::record(s);
				return static_cast<T*>(ptr);
			}
		}
		throw std::bad_alloc();
	}
	void deallocate(T* ptr, std::size_t n)
	{
		std::free(ptr);
		std::size_t s = n * sizeof(T);
		memory_monitor::record(-((int64_t)s));
	}
};

template <typename T, typename U>
inline bool operator==(const track_allocator<T>&, const track_allocator<U>&)
{
	return true;
}

template <typename T, typename U>
inline bool operator!=(const track_allocator<T>& a, const track_allocator<U>& b)
{
	return !(a == b);
}

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
			<< duration_cast<milliseconds>(ev.allocations[j].timestamp - m.start_log).count() << ","
			<< ev.allocations[j].usage << "]";
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
