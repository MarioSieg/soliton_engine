
const top_margin = 20;
const item_height = 26;
const indent_width = 12;
const tree_width = 215;

function subtree_height(entity_data) {
  let result = item_height;

  if (entity_data.expand) {
    for (let e in entity_data.entities) {
      const nested = entity_data.entities[e];
      result += this.subtree_height(nested);
    }
  }

  return result;
}

Vue.component('entity-tree-item', {
  props: ['x', 'y', 'entity_data', 'selected_entity', 'width'],
  data: function() {
    return {
      text_elem: undefined
    }
  },
  methods: {
    toggle: function() {
      this.$emit('toggle', this.entity_data);
    },
    search: function() {
      this.$emit('select_query', this.entity_data.path);
    },
    select: function() {
      this.$emit('select', this.entity_data);
    },
  },
  computed: {
    css_select_box: function() {
      if (this.entity_data.path == this.selected_entity) {
        return "entity-tree-select entity-tree-selected";
      } else {
        return "entity-tree-select";
      }
    },
    width_select_box: function() {
      return Math.max(this.width - this.x - 29, 0);
    },
    css_text: function() {
      let result = "entity-tree-text";
      if (this.entity_data.path == this.selected_entity) {
        result += " entity-tree-text-select noselect";
      }
      if (this.entity_data.is_disabled) {
        result += " entity-tree-text-disabled";
      }
      return result;
    },
    xp: function() {
      return this.x + 3;
    },
    entity_label: function() {
      let result = this.entity_data.label;
      let prefab = this.entity_data.prefab;
      if (prefab != "*") {
        let dot_sep = prefab.split(".");
        prefab = dot_sep[dot_sep.length - 1];
        result += " : " + prefab;
      }
      return result;
    }
  },
  template: `
    <svg>
      <rect :x="xp + 28" :y="y - 16" :width="width_select_box" height="23px" :class="css_select_box"
        v-on:click="select">
      </rect>

      <template v-if="entity_data.has_children">
        <rect :x="xp" :y="y - 5" :width="5" height="1" fill="#44464D"></rect>
        <image v-if="!entity_data.expand"
          class="entity-tree-icon" 
          href="img/nav-right.png" 
          :x="xp + 2" :y="y - 12" 
          v-on:click="toggle">
        </image>
        <image v-else
          class="entity-tree-icon" 
          href="img/nav-down.png" 
          :x="xp + 2" 
          :y="y - 12" 
          v-on:click="toggle">
        </image>
      </template>
      <template v-else>
        <rect :x="xp" :y="y - 5" :width="15" height="1" fill="#44464D"></rect>
      </template>

      <entity-icon :x="xp + 17" :y="y - 8" :entity_data="entity_data"></entity-icon>

      <text :class="css_text" :x="xp + 33" :y="y" v-on:click="select" ref="item_text">{{entity_label}}</text>
    </svg>`
});

Vue.component('entity-tree-outline', {
  props: ['x', 'y', 'entity_data'],
  data: function() {
    return {
      expand: false
    }
  },
  methods: {
    height: function() {
      let result = subtree_height(this.entity_data) - item_height - 7;
      if (result < 0) {
        result = 0;
      }
      return result;
    }
  },
  template: `
    <svg>
      <rect :x="x + 3" :y="y + 2" :width="1" :height="height()" fill="#44464D"></rect>
    </svg>`
});

Vue.component('entity-tree-list', {
  props: ['entities', 'x', 'y', 'selected_entity', 'width'],
  data: function() {
    return {
      expand: false
    }
  },
  computed: {
    sorted_entities: function() {
      let result = [];
      for (const entity in this.entities) {
        result.push(this.entities[entity]);
      }

      result.sort((e1, e2) => {
        if (e1.is_module == e2.is_module) {
          if (e1.is_prefab == e2.is_prefab) {
            if (e1.has_children == e2.has_children) {
              if (e1.is_component == e2.is_component) {
                if (e1.label && e2.label) {
                  return e1.label.localeCompare(e2.label);
                } else {
                  return e1.name.localeCompare(e2.name);
                }
              } else {
                if (e1.is_component) {
                  return -1;
                } else {
                  return 1;
                }
              }
            } else {
              if (e1.has_children) {
                return -1;
              } else {
                return 1;
              }
            }
          } else {
            if (e1.is_prefab) {
              return -1;
            } else {
              return 1;
            }
          }
        } else {
          if (e1.is_module) {
            return -1;
          } else {
            return 1;
          }
        }
      });

      return result;
    }
  },
  methods: {
    toggle: function(entity) {
      this.$emit('toggle', entity);
    },
    select: function(entity) {
      this.$emit('select', entity);
    },
    select_query: function(entity) {
      this.$emit('select_query', entity);
    },
    item_y: function(item) {
      return this.y + (item * item_height);
    },
    entities_to_elems: function(h, entities) {
      let children = [];
      let height = this.y;
  
      for (let i = 0; i < entities.length; i ++) {
        const entity_data = entities[i];
        let elem = h('entity-tree-item', {
          props: {
            x: this.x,
            y: height,
            entity_data: entity_data,
            selected_entity: this.selected_entity,
            width: this.width
          },
  
          on: {
            toggle: this.toggle,
            select: this.select,
            select_query: this.select_query,
          }
        });
  
        children.push(elem);

        if (entity_data.expand) {
          const nested_entities = entity_data.entities;
          const outline_elem = h('entity-tree-outline', {
            props: {
              x: this.x + indent_width,
              y: height,
              entity_data: entity_data
            }
          });
          children.push(outline_elem);
          
          const list_elem = h('entity-tree-list', {
            props: {
              entities: nested_entities,
              x: this.x + indent_width,
              y: height + item_height,
              selected_entity: this.selected_entity,
              width: this.width
            },
            on: {
              toggle: this.toggle,
              select: this.select,
              select_query: this.select_query
            }
          });
          children.push(list_elem);

          height += subtree_height(entity_data);
        } else {
          height += item_height;
        }
      }

      return children;
    }
  },
  render: function(h) {
    return h('svg', this.entities_to_elems(h, this.sorted_entities));
  }
});

Vue.component('entity-tree', {
  props: ['valid'],
  data: function() {
    return {
      width: 0,
      disabled: false,
      selected_entity: undefined,
      root: {
        path: "0",
        entities: {},
        expand: true,
        selection: undefined
      }
    }
  },
  beforeUpdate: function() {
    this.width = this.$el.clientWidth;
  },
  methods: {
    get_name: function(path) {
      return path.split('.').pop();
    },
    update_scope: function(scope, data) {
      // Store entities in new scope, so that deleted entities are automatically
      // cleaned up
      let result = {};

      if (data && data.results) {
        for (var r = 0; r < data.results.length; r ++) {
          const elem = data.results[r];
          for (var e = 0; e < elem.entities.length; e ++) {
            const name = elem.entities[e] + "";
            const name_esc = name.replaceAll(".", "\\.");
            let path = name_esc
            if (elem.parent) {
              path = elem.parent + "." + name_esc
            }

            let label;
            let color;
            if (elem.entity_labels) {
              label = elem.entity_labels[e];
            }
            if (elem.colors) {
              color = elem.colors[e];
              if (!color) {
                color = undefined;
              }
            }
            if (!label) {
              label = name;
            }

            let entity = scope[name];
            if (!entity) {
              entity = {
                expand: false,
                name: name,
                path: path,
                entities: {},
                type: elem.type
              };
            }

            entity.label = label;
            entity.color = color;
            entity.has_children = elem.is_set[5];
            entity.is_module = elem.is_set[1];
            entity.is_component = elem.is_set[2];
            entity.is_prefab = elem.is_set[3];
            entity.is_disabled = elem.is_set[4];
            entity.prefab = elem.vars[0];

            Vue.set(result, name, entity);
          }
        }
      }

      if (this.selection && scope[this.selection.name] != undefined) {
        if (!result[this.selection.name]) {
          // Selected entity is no longer available, clear it
          this.$emit('select');
        }
      }

      return result;
    },
    update: function(container, onready) {
      if (!container) {
        container = this.root;
      }

      let path = container.path;
      path = path.replaceAll(" ", "\\ ");

      const q = "(ChildOf, " + path + "), ?Module, ?Component, ?Prefab, ?Disabled, ?ChildOf(_, $this), ?IsA($this, $base:self)";
      app.request_query('tree-' + container.path, q, (reply) => {
        if (reply.error) {
          console.error("treeview: " + reply.error);
        } else {
          container.entities = this.update_scope(container.entities, reply);
          if (onready) {
            onready();
          }
        }
      }, undefined, {
        values: false, 
        ids: false, 
        term_ids: false, 
        sources: false,
        entity_labels: true,
        variable_labels: true,
        colors: true,
        is_set: true,
        prefab: undefined
      });
    },
    update_expanded: function(container) {
      if (this.disabled) {
        return;
      }

      if (!container) {
        container = this.root;
      }

      this.update(container, () => {
        for (const entity in container.entities) {
          const entity_data = container.entities[entity];
          if (entity_data.expand) {
            this.update_expanded(entity_data);
          }
        }
      });
    },
    toggle: function(entity) {
      entity.expand = !entity.expand;
      if (entity.expand) {
        this.update(entity);
      }
    },
    collapse_all: function(cur) {
      if (!cur) {
        cur = this.root;
      }

      app.request_abort('tree-' + cur.path);

      for (let k in cur.entities) {
        let ent = cur.entities[k];
        this.collapse_all(ent);
        
        ent.expand = false;
        ent.entities = {};
      }
    },
    select_recursive(entity, cur, elems, i, onready) {
      if (!cur) {
        return;
      }

      this.update(cur, () => {
        cur.expand = true;

        let next = cur.entities[elems[i]];
        if (!next) {
          this.collapse_all();
        }

        if (i < (elems.length - 1)) {
          this.select_recursive(entity, next, elems, i + 1, onready);
        } else if (onready) {
          onready(next);
        }
      });
    },
    select: function(entity) {
      if (!entity) {
        this.$emit('select');
        return;
      }

      const elems = entity.split('.');
      let cur = this.root;

      this.collapse_all();

      this.select_recursive(entity, cur, elems, 0, (item) => {
        if (entity != this.selected_entity) {
          this.evt_select(item);
        }
      });
    },
    set_selected_entity(entity) {
      this.selected_entity = entity;
    },
    evt_select: function(entity) {
      if (entity) {
        if (entity.path == this.selected_entity) {
          this.$emit('select'); // Toggle
        } else {
          this.$emit('select', entity);
        }
      } else {
        this.$emit('select');
      }
    },
    evt_select_query: function(entity) {
      this.$emit('select_query', entity);
    },
    open: function() {
      this.disabled = false;
      this.$emit("panel-update");
      this.update_expanded();
    },
    close: function() {
      this.disabled = true;
      this.$emit("panel-update");
    }
  },
  computed: {
    entity_count: function() {
      return this.root.entities.length;
    },
    tree_height: function() {
      return subtree_height(this.root) + 100;
    },
    tree_top_margin: function() {
      return top_margin;
    },
    css: function() {
      let result = "entity-tree";
      if (!this.valid) {
        result += " invalid";
      }
      if (this.disabled) {
        result += " disable";
      }
      return result;
    }
  },
  template: `
    <div :class="css">
      <svg :height="tree_height" width="100%">
        <entity-tree-list 
          :entities="root.entities" 
          :x="0"
          :y="tree_top_margin" 
          :width="width"
          :selected_entity="selected_entity"
          v-on:toggle="toggle"
          v-on:select="evt_select"
          v-on:select_query="evt_select_query">
        </entity-tree-list>
      </svg>
    </div>
    `
});
