/**
 * A software rasterizer over an ImageData (without WebGL). It receives a scene
 * with every value already computed by the compiler.
 *
 * This file is embedded in the compiler when CMake configures the project, so
 * it must not contain the closing tag of a script block.
 */
const Runtime = (function () {
	const AMBIENT = 0.12;
	const BACKGROUND = [16, 20, 24];
	const NEAR = 0.1;
	const SPHERE_SLICES = 48;
	const SPHERE_STACKS = 32;

	/** Vectors. */

	const add = (a, b) => [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
	const subtract = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
	const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	const cross = (a, b) => [
		a[1] * b[2] - a[2] * b[1],
		a[2] * b[0] - a[0] * b[2],
		a[0] * b[1] - a[1] * b[0]
	];
	const lerp = (a, b, t) => [
		a[0] + (b[0] - a[0]) * t,
		a[1] + (b[1] - a[1]) * t,
		a[2] + (b[2] - a[2]) * t
	];
	const normalize = (a) => {
		const length = Math.sqrt(dot(a, a));
		return length === 0 ? a : [a[0] / length, a[1] / length, a[2] / length];
	};

	/** Matrices (row-major). */

	const transformPoint = (m, p) => [
		m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3],
		m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7],
		m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11]
	];

	/** The inverse transpose of the upper-left 3x3 block (the scale is not important, because the normals are normalized). */
	const normalMatrix = (m) => [
		m[5] * m[10] - m[6] * m[9], m[6] * m[8] - m[4] * m[10], m[4] * m[9] - m[5] * m[8],
		m[2] * m[9] - m[1] * m[10], m[0] * m[10] - m[2] * m[8], m[1] * m[8] - m[0] * m[9],
		m[1] * m[6] - m[2] * m[5], m[2] * m[4] - m[0] * m[6], m[0] * m[5] - m[1] * m[4]
	];

	const transformNormal = (n, v) => normalize([
		n[0] * v[0] + n[1] * v[1] + n[2] * v[2],
		n[3] * v[0] + n[4] * v[1] + n[5] * v[2],
		n[6] * v[0] + n[7] * v[1] + n[8] * v[2]
	]);

	/** Tessellation: every mesh is a list of vertices and triangles, in local coordinates. */

	function tessellateBox(size) {
		const [w, h, d] = [size[0] / 2, size[1] / 2, size[2] / 2];
		const faces = [
			{ normal: [1, 0, 0], corners: [[w, -h, d], [w, -h, -d], [w, h, -d], [w, h, d]] },
			{ normal: [-1, 0, 0], corners: [[-w, -h, -d], [-w, -h, d], [-w, h, d], [-w, h, -d]] },
			{ normal: [0, 1, 0], corners: [[-w, h, d], [w, h, d], [w, h, -d], [-w, h, -d]] },
			{ normal: [0, -1, 0], corners: [[-w, -h, -d], [w, -h, -d], [w, -h, d], [-w, -h, d]] },
			{ normal: [0, 0, 1], corners: [[-w, -h, d], [w, -h, d], [w, h, d], [-w, h, d]] },
			{ normal: [0, 0, -1], corners: [[w, -h, -d], [-w, -h, -d], [-w, h, -d], [w, h, -d]] }
		];
		const vertices = [];
		const triangles = [];
		for (const face of faces) {
			const base = vertices.length;
			for (const position of face.corners) {
				vertices.push({ position, normal: face.normal });
			}
			triangles.push([base, base + 1, base + 2], [base, base + 2, base + 3]);
		}
		return { vertices, triangles };
	}

	function tessellatePlane(size) {
		const [w, d] = [size[0] / 2, size[1] / 2];
		const normal = [0, 1, 0];
		const vertices = [[-w, 0, d], [w, 0, d], [w, 0, -d], [-w, 0, -d]].map((position) => ({ position, normal }));
		return { vertices, triangles: [[0, 1, 2], [0, 2, 3]] };
	}

	function tessellateSphere(size) {
		const radius = size[0];
		const vertices = [];
		const triangles = [];
		for (let stack = 0; stack <= SPHERE_STACKS; ++stack) {
			const polar = Math.PI * stack / SPHERE_STACKS;
			for (let slice = 0; slice <= SPHERE_SLICES; ++slice) {
				const azimuth = 2 * Math.PI * slice / SPHERE_SLICES;
				const normal = [
					Math.sin(polar) * Math.cos(azimuth),
					Math.cos(polar),
					Math.sin(polar) * Math.sin(azimuth)
				];
				vertices.push({ position: [normal[0] * radius, normal[1] * radius, normal[2] * radius], normal });
			}
		}
		const row = SPHERE_SLICES + 1;
		for (let stack = 0; stack < SPHERE_STACKS; ++stack) {
			for (let slice = 0; slice < SPHERE_SLICES; ++slice) {
				const a = stack * row + slice;
				const b = a + row;
				if (stack !== 0) {
					triangles.push([a, a + 1, b]);
				}
				if (stack !== SPHERE_STACKS - 1) {
					triangles.push([a + 1, b + 1, b]);
				}
			}
		}
		return { vertices, triangles };
	}

	function tessellate(mesh) {
		switch (mesh.type) {
			case "box": return tessellateBox(mesh.size);
			case "plane": return tessellatePlane(mesh.size);
			default: return tessellateSphere(mesh.size);
		}
	}

	/** Camera. */

	function createCamera(camera, width, height) {
		const forward = normalize(subtract(camera.target, camera.position));
		// If the camera looks vertically, the usual up vector is not useful.
		const up = Math.abs(forward[1]) > 0.999 ? [0, 0, 1] : [0, 1, 0];
		const right = normalize(cross(forward, up));
		return {
			position: camera.position,
			right,
			up: cross(right, forward),
			forward,
			focal: 1 / Math.tan(camera.fov * Math.PI / 360),
			aspect: width / height,
			width,
			height
		};
	}

	/** Coordinates in the camera space, where "z" is the distance in front of the camera. */
	function toCameraSpace(camera, point) {
		const relative = subtract(point, camera.position);
		return [dot(relative, camera.right), dot(relative, camera.up), dot(relative, camera.forward)];
	}

	/** Clipping against the near plane (Sutherland-Hodgman). */
	function clipNear(triangle) {
		const polygon = [];
		for (let k = 0; k < 3; ++k) {
			const current = triangle[k];
			const next = triangle[(k + 1) % 3];
			const isCurrentInside = current.view[2] >= NEAR;
			const isNextInside = next.view[2] >= NEAR;
			if (isCurrentInside) {
				polygon.push(current);
			}
			if (isCurrentInside !== isNextInside) {
				const t = (NEAR - current.view[2]) / (next.view[2] - current.view[2]);
				polygon.push({
					view: lerp(current.view, next.view, t),
					world: lerp(current.world, next.world, t),
					normal: lerp(current.normal, next.normal, t)
				});
			}
		}
		return polygon;
	}

	/** The vertex in screen coordinates, with its attributes. */
	function project(camera, vertex) {
		const inverseDepth = 1 / vertex.view[2];
		return {
			x: (vertex.view[0] * camera.focal * inverseDepth / camera.aspect + 1) * camera.width / 2,
			y: (1 - vertex.view[1] * camera.focal * inverseDepth) * camera.height / 2,
			inverseDepth,
			world: vertex.world,
			normal: vertex.normal
		};
	}

	/** Blinn-Phong shading per pixel. The result is in the range [0, 255]. */
	function shade(context, material, position, normal) {
		const color = material.color;
		const view = normalize(subtract(context.camera.position, position));
		let n = normalize(normal);
		if (dot(n, view) < 0) {
			n = [-n[0], -n[1], -n[2]];
		}
		let r = AMBIENT * color[0];
		let g = AMBIENT * color[1];
		let b = AMBIENT * color[2];
		for (const light of context.lights) {
			const direction = light.type === "directional"
				? [-light.direction[0], -light.direction[1], -light.direction[2]]
				: normalize(subtract(light.position, position));
			const diffuse = dot(n, direction);
			if (diffuse <= 0) {
				continue;
			}
			let specular = 0;
			if (material.shininess > 0) {
				const halfway = normalize(add(direction, view));
				specular = Math.pow(Math.max(0, dot(n, halfway)), material.shininess);
			}
			const power = light.intensity;
			r += (diffuse * color[0] + specular) * light.color[0] * power;
			g += (diffuse * color[1] + specular) * light.color[1] * power;
			b += (diffuse * color[2] + specular) * light.color[2] * power;
		}
		return [Math.min(1, r) * 255, Math.min(1, g) * 255, Math.min(1, b) * 255];
	}

	/** The top-left rule, so neighbour triangles don't draw their shared edge twice. */
	const isTopLeft = (a, b) => (b.y === a.y && b.x > a.x) || b.y < a.y;

	/** Draws a triangle in screen coordinates, with the z-buffer and perspective-correct interpolation. */
	function rasterize(context, material, v0, v1, v2) {
		let area = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
		if (area === 0) {
			return;
		}
		if (area < 0) {
			[v1, v2] = [v2, v1];
			area = -area;
		}
		const { width, height, pixels, depth } = context;
		const alpha = material.color[3];
		const isOpaque = alpha >= 1;
		const minX = Math.max(0, Math.floor(Math.min(v0.x, v1.x, v2.x)));
		const maxX = Math.min(width - 1, Math.ceil(Math.max(v0.x, v1.x, v2.x)));
		const minY = Math.max(0, Math.floor(Math.min(v0.y, v1.y, v2.y)));
		const maxY = Math.min(height - 1, Math.ceil(Math.max(v0.y, v1.y, v2.y)));
		const topLeft0 = isTopLeft(v1, v2);
		const topLeft1 = isTopLeft(v2, v0);
		const topLeft2 = isTopLeft(v0, v1);
		for (let y = minY; y <= maxY; ++y) {
			const py = y + 0.5;
			for (let x = minX; x <= maxX; ++x) {
				const px = x + 0.5;
				const e0 = (v2.x - v1.x) * (py - v1.y) - (v2.y - v1.y) * (px - v1.x);
				const e1 = (v0.x - v2.x) * (py - v2.y) - (v0.y - v2.y) * (px - v2.x);
				const e2 = (v1.x - v0.x) * (py - v0.y) - (v1.y - v0.y) * (px - v0.x);
				if (e0 < 0 || e1 < 0 || e2 < 0
					|| (e0 === 0 && !topLeft0)
					|| (e1 === 0 && !topLeft1)
					|| (e2 === 0 && !topLeft2)) {
					continue;
				}
				const w0 = e0 / area * v0.inverseDepth;
				const w1 = e1 / area * v1.inverseDepth;
				const w2 = e2 / area * v2.inverseDepth;
				const inverseDepth = w0 + w1 + w2;
				const index = y * width + x;
				if (inverseDepth <= depth[index]) {
					continue;
				}
				const position = [
					(w0 * v0.world[0] + w1 * v1.world[0] + w2 * v2.world[0]) / inverseDepth,
					(w0 * v0.world[1] + w1 * v1.world[1] + w2 * v2.world[1]) / inverseDepth,
					(w0 * v0.world[2] + w1 * v1.world[2] + w2 * v2.world[2]) / inverseDepth
				];
				const normal = [
					w0 * v0.normal[0] + w1 * v1.normal[0] + w2 * v2.normal[0],
					w0 * v0.normal[1] + w1 * v1.normal[1] + w2 * v2.normal[1],
					w0 * v0.normal[2] + w1 * v1.normal[2] + w2 * v2.normal[2]
				];
				const color = shade(context, material, position, normal);
				const offset = 4 * index;
				if (isOpaque) {
					depth[index] = inverseDepth;
					pixels[offset] = color[0];
					pixels[offset + 1] = color[1];
					pixels[offset + 2] = color[2];
				}
				else {
					pixels[offset] = color[0] * alpha + pixels[offset] * (1 - alpha);
					pixels[offset + 1] = color[1] * alpha + pixels[offset + 1] * (1 - alpha);
					pixels[offset + 2] = color[2] * alpha + pixels[offset + 2] * (1 - alpha);
				}
			}
		}
	}

	function drawTriangle(context, triangle) {
		const polygon = clipNear(triangle.vertices).map((vertex) => project(context.camera, vertex));
		for (let k = 1; k + 1 < polygon.length; ++k) {
			rasterize(context, triangle.material, polygon[0], polygon[k], polygon[k + 1]);
		}
	}

	/** Transforms a mesh to the world and camera spaces, and returns its triangles. */
	function createTriangles(camera, mesh) {
		const geometry = tessellate(mesh);
		const normals = normalMatrix(mesh.model);
		const vertices = geometry.vertices.map((vertex) => {
			const world = transformPoint(mesh.model, vertex.position);
			return {
				world,
				view: toCameraSpace(camera, world),
				normal: transformNormal(normals, vertex.normal)
			};
		});
		return geometry.triangles.map((indices) => {
			const triangle = indices.map((index) => vertices[index]);
			return {
				material: mesh,
				vertices: triangle,
				distance: (triangle[0].view[2] + triangle[1].view[2] + triangle[2].view[2]) / 3
			};
		});
	}

	function render(canvas, scene) {
		const width = canvas.width;
		const height = canvas.height;
		const graphics = canvas.getContext("2d");
		const image = graphics.createImageData(width, height);
		const pixels = image.data;
		for (let offset = 0; offset < pixels.length; offset += 4) {
			pixels[offset] = BACKGROUND[0];
			pixels[offset + 1] = BACKGROUND[1];
			pixels[offset + 2] = BACKGROUND[2];
			pixels[offset + 3] = 255;
		}
		const camera = createCamera(scene.camera, width, height);
		const context = {
			camera,
			lights: scene.lights,
			width,
			height,
			pixels,
			// It holds 1/z, so 0 is the infinite distance and the greatest value wins.
			depth: new Float64Array(width * height)
		};
		const translucent = [];
		for (const mesh of scene.meshes) {
			const triangles = createTriangles(camera, mesh);
			if (mesh.color[3] < 1) {
				translucent.push(...triangles);
			}
			else {
				triangles.forEach((triangle) => drawTriangle(context, triangle));
			}
		}
		translucent.sort((a, b) => b.distance - a.distance);
		translucent.forEach((triangle) => drawTriangle(context, triangle));
		graphics.putImageData(image, 0, 0);
	}

	return { render };
})();
